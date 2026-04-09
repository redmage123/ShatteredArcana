// Copyright Mythforge Studios. All Rights Reserved.
// CoMCommandSerializer.cpp — FCoMCommandPayload <-> wire format (compact binary)
// Owner: Network Architect
//
// Wire format (version 0x01) — fixed-width fields, little-endian:
//
//   Offset  Size   Field
//   ------  ----   -----
//   0       1      uint8  — format version (= 0x01)
//   1       1      uint8  — ECoMCommandType (cast from underlying uint8)
//   2       4      int32  — WizardIndex
//   6       4      int32  — TurnNumber
//   10      4      int32  — TargetID_A
//   14      4      int32  — TargetID_B
//   18      1      uint8  — ECoMPlane (TargetPlane, cast from underlying uint8)
//   19      2      uint16 — AssetRef UTF-8 byte length (0 = NAME_None)
//   21      N      utf8   — AssetRef string bytes (variable, may be 0)
//   21+N    4      int32  — Quantity
//   25+N    4      int32  — PayloadChecksum
//
// FName is serialized as a length-prefixed UTF-8 string for cross-machine
// portability. Never use FArchive << FName directly on the wire — FName indices
// are per-binary and not portable between different builds or machines.
//
// No protobuf, no external deps. FMemoryWriter/FMemoryReader only.

#include "CoMCommandSerializer.h"
#include "CoMCore/Turn/CoMCommandQueue.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Internationalization/Text.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMSerializer, Log, All);

namespace CoMWire
{
    // Bump this when the wire format changes. Old clients will reject new packets.
    static constexpr uint8 FormatVersion = 0x01;

    // Largest AssetRef string we accept on deserialization.
    // FName strings top out at NAME_SIZE (1024) chars; wire cap matches.
    static constexpr uint16 MaxAssetRefBytes = 1024;
}

// ─────────────────────────────────────────────────────────────────────────────
// Serialize
// ─────────────────────────────────────────────────────────────────────────────

TArray<uint8> UCoMCommandSerializer::Serialize(const FCoMCommandPayload& Payload)
{
    // Pre-allocate to avoid repeated heap growth: fixed fields = 25 bytes + string.
    TArray<uint8> Buffer;
    Buffer.Reserve(32);

    FMemoryWriter Writer(Buffer, /*bIsPersistent=*/true);

    // ── Version tag ──────────────────────────────────────────────────────────
    uint8 Version = CoMWire::FormatVersion;
    Writer << Version;

    // ── Command type (uint8 cast — UENUM underlying type is uint8) ───────────
    uint8 CmdType = static_cast<uint8>(Payload.CommandType);
    Writer << CmdType;

    // ── Integer fields ────────────────────────────────────────────────────────
    // Local copies so Writer << can take a non-const ref without modifying Payload.
    int32 WizardIndex = Payload.WizardIndex;
    int32 TurnNumber  = Payload.TurnNumber;
    int32 TargetA     = Payload.TargetID_A;
    int32 TargetB     = Payload.TargetID_B;
    Writer << WizardIndex;
    Writer << TurnNumber;
    Writer << TargetA;
    Writer << TargetB;

    // ── Plane (uint8 cast) ────────────────────────────────────────────────────
    uint8 Plane = static_cast<uint8>(Payload.TargetPlane);
    Writer << Plane;

    // ── AssetRef as length-prefixed UTF-8 ────────────────────────────────────
    // NAME_None → zero-length string.
    const FString AssetStr = (Payload.AssetRef == NAME_None)
        ? FString()
        : Payload.AssetRef.ToString();

    FTCHARToUTF8 Utf8(*AssetStr);
    const int32 Utf8Len = Utf8.Length();
    check(Utf8Len <= CoMWire::MaxAssetRefBytes); // asset names must be short

    uint16 StrLen = static_cast<uint16>(Utf8Len);
    Writer << StrLen;
    if (StrLen > 0)
    {
        // Serialize raw bytes — FMemoryWriter::Serialize is the correct path
        // for pre-converted byte data (no endian swap on raw bytes).
        Writer.Serialize(
            const_cast<ANSICHAR*>(Utf8.Get()),
            static_cast<int64>(StrLen));
    }

    // ── Remaining integer fields ──────────────────────────────────────────────
    int32  Qty      = Payload.Quantity;
    int32  Checksum = Payload.PayloadChecksum;
    Writer << Qty;
    Writer << Checksum;

    return Buffer;
}

// ─────────────────────────────────────────────────────────────────────────────
// Deserialize
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMCommandSerializer::Deserialize(const TArray<uint8>& Wire, FCoMCommandPayload& OutPayload)
{
    // Minimum possible packet: version(1) + cmdtype(1) + 4×int32(16) + plane(1) + strlen(2) + quantity(4) + checksum(4) = 29
    static constexpr int32 MinPacketBytes = 29;
    if (Wire.Num() < MinPacketBytes)
    {
        UE_LOG(LogCoMSerializer, Warning,
            TEXT("Deserialize: wire buffer too short (%d bytes, minimum %d)."),
            Wire.Num(), MinPacketBytes);
        return false;
    }

    FMemoryReader Reader(Wire, /*bIsPersistent=*/true);

    // ── Version check ─────────────────────────────────────────────────────────
    uint8 Version = 0;
    Reader << Version;
    if (Version != CoMWire::FormatVersion)
    {
        UE_LOG(LogCoMSerializer, Warning,
            TEXT("Deserialize: unknown wire format version 0x%02X (expected 0x%02X). "
                 "Reject. Client/server version mismatch?"),
            Version, CoMWire::FormatVersion);
        return false;
    }

    // ── Command type ──────────────────────────────────────────────────────────
    uint8 CmdType = 0;
    Reader << CmdType;
    if (CmdType >= static_cast<uint8>(ECoMCommandType::MAX))
    {
        UE_LOG(LogCoMSerializer, Warning,
            TEXT("Deserialize: invalid CommandType %d (MAX=%d)."),
            CmdType, static_cast<int32>(ECoMCommandType::MAX));
        return false;
    }
    OutPayload.CommandType = static_cast<ECoMCommandType>(CmdType);

    // ── Integer fields ────────────────────────────────────────────────────────
    Reader << OutPayload.WizardIndex;
    if (OutPayload.WizardIndex < 0 || OutPayload.WizardIndex >= CoM::MAX_WIZARDS)
    {
        UE_LOG(LogCoMSerializer, Warning, TEXT("Invalid WizardIndex %d"), OutPayload.WizardIndex);
        return false;
    }
    Reader << OutPayload.TurnNumber;
    Reader << OutPayload.TargetID_A;
    Reader << OutPayload.TargetID_B;

    // ── Plane ─────────────────────────────────────────────────────────────────
    uint8 Plane = 0;
    Reader << Plane;
    if (Plane >= static_cast<uint8>(ECoMPlane::MAX))
    {
        UE_LOG(LogCoMSerializer, Warning, TEXT("Invalid plane value %d in deserialized packet"), Plane);
        return false;
    }
    OutPayload.TargetPlane = static_cast<ECoMPlane>(Plane);

    // ── AssetRef (length-prefixed UTF-8) ──────────────────────────────────────
    uint16 StrLen = 0;
    Reader << StrLen;

    if (StrLen == 0)
    {
        OutPayload.AssetRef = NAME_None;
    }
    else
    {
        if (StrLen > CoMWire::MaxAssetRefBytes)
        {
            UE_LOG(LogCoMSerializer, Warning,
                TEXT("Deserialize: AssetRef length %d exceeds max %d. Likely corrupt/malicious packet."),
                StrLen, CoMWire::MaxAssetRefBytes);
            return false;
        }

        // Read raw UTF-8 bytes, null-terminate for FUTF8ToTCHAR.
        TArray<uint8> StrBytes;
        StrBytes.SetNumUninitialized(static_cast<int32>(StrLen) + 1);
        Reader.Serialize(StrBytes.GetData(), static_cast<int64>(StrLen));
        StrBytes[StrLen] = 0; // null terminator

        FUTF8ToTCHAR Tchar(reinterpret_cast<const ANSICHAR*>(StrBytes.GetData()), StrLen);
        OutPayload.AssetRef = FName(Tchar.Get());
    }

    // ── Remaining fields ──────────────────────────────────────────────────────
    Reader << OutPayload.Quantity;
    Reader << OutPayload.PayloadChecksum;

    if (Reader.IsError())
    {
        UE_LOG(LogCoMSerializer, Warning,
            TEXT("Deserialize: FMemoryReader error after parsing all fields. "
                 "Packet truncated or corrupt."));
        // Zero-out partially read payload to avoid feeding bad state upstream.
        OutPayload = FCoMCommandPayload{};
        return false;
    }

    return true;
}
