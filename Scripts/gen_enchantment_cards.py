"""Generate illuminated tarot-card art for every global enchantment with SDXL
on the dev server (176.9.99.103).

One ornate gold-bordered card per enchantment, portrait 832x1216, dropped in
~/enchant-cards/enchant_<slug>.png. The slug matches the SpellID mapping used
by CoMGlobalEnchantmentData so the UE import step can wire each card to its
spell automatically.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/enchant-cards")
os.makedirs(OUT_DIR, exist_ok=True)

# Portrait card aspect (~2:3), SDXL-friendly dimensions.
W, H = 832, 1216
STEPS = 34
CFG = 7.5

# Note: we WANT an ornate frame here, so "frame"/"border" are intentionally
# NOT in the negative prompt (unlike the wizard backgrounds).
NEG = (
    "ugly, deformed, disfigured, mutated, low quality, low resolution, blurry, "
    "jpeg artifacts, watermark, signature, modern, photo, photograph, 3d render, "
    "cgi, plastic, bad anatomy, extra limbs, extra fingers, out of frame, cropped, "
    "duplicate, gibberish text, misspelled text"
)

# Illuminated tarot-card frame, locked across all cards for a consistent set.
STYLE = (
    "ornate illuminated tarot card, thick gold filigree border inlaid with "
    "realm-colored gemstones, decorative engraved title banner at the bottom, "
    "arcane sigils in the four corners, symmetrical card composition, symbolic "
    "centerpiece illustration, rich high-fantasy oil painting, mystical, "
    "luminous, gilt and parchment, museum-quality fine art, intricate detail"
)

# (slug, display, realm-color-fragment, centerpiece, seed)
CARDS = [
    # --- Life ---
    ("just_cause", "Just Cause",
     "warm gold and pure white holy light",
     "radiant golden scales of justice balanced above a sunlit kingdom, white "
     "doves and bright banners, an aura of divine righteous order", 5301),
    ("crusade", "Crusade",
     "warm gold and white holy light",
     "a holy paladin's gauntlet raising a glowing sword crowned with a halo, "
     "ranks of armored crusaders marching beneath golden banners", 5302),
    ("tranquility", "Tranquility",
     "soft white and pale blue serene light",
     "a serene white dove gliding over a calm moonlit lake, a glowing lotus, a "
     "gentle aura dissolving dark storm clouds into peace", 5303),
    ("consecration", "Consecration",
     "warm gold and ivory sacred light",
     "a consecrated stone altar bathed in pillars of golden light, sacred "
     "glowing sigils, a blessed and hallowed sanctuary", 5304),
    # --- Death ---
    ("death_wish", "Death Wish",
     "sickly green and deep violet deathlight",
     "a skeletal hand crushing a fading glowing soul, doomed figures kneeling in "
     "a pall of green deathlight, an aura of inescapable doom", 5311),
    ("eternal_night", "Eternal Night",
     "cold violet and bruised black, blood-red moon",
     "a black sun eclipsing the world into eternal starless darkness, pale "
     "undead rising under a blood-red moon", 5312),
    ("zombie_mastery", "Zombie Mastery",
     "sickly green and grave-brown",
     "rotting zombies clawing up from cracked graveyard earth under a "
     "necromancer's green glow, bound to a grinning skull sigil", 5313),
    # --- Chaos ---
    ("call_the_void", "Call the Void",
     "deep black and searing red",
     "a swirling black void tearing open the sky, reality crumbling into "
     "nothingness, faint screaming faces dissolving into the abyss", 5321),
    ("armageddon_clock", "Armageddon Clock",
     "molten orange and brass and ash-grey",
     "an infernal brass doomsday clock with molten glowing hands creeping toward "
     "midnight, volcanoes erupting on the horizon behind it", 5322),
    ("chaos_surge", "Chaos Surge",
     "violent red and orange",
     "a chaotic explosion of crimson lightning and warped flame, shattered runes "
     "scattering, unstable raw magic surging outward", 5323),
    ("great_wasting", "Great Wasting",
     "ash-grey, scorched red, dead brown",
     "a blasted hellscape of cracked scorched earth and skeletal dead trees, ash "
     "storms sweeping under a dying swollen red sun", 5324),
    ("doom_mastery", "Doom Mastery",
     "black and infernal red",
     "a towering horned demon lord wreathed in black fire commanding lesser "
     "fiends, a burning doom sigil hovering above its brow", 5325),
    # --- Nature ---
    ("natures_wrath", "Nature's Wrath",
     "deep forest green and storm-grey",
     "a colossal enraged forest guardian of root, bark and stone, vines and "
     "thorns erupting, a lightning-split sky, beasts charging", 5331),
    ("nature_awareness", "Nature Awareness",
     "emerald green and earthy gold",
     "a great glowing emerald eye formed of woven leaves and vines, overlooking a "
     "living map of forests, rivers and mountains", 5332),
    ("herb_mastery", "Herb Mastery",
     "lush green and golden",
     "an abundance of glowing healing herbs and blossoming flowers spilling from "
     "a druid's woven basket, golden pollen and fertile growth", 5333),
    ("gaia_force", "Gaia Force",
     "verdant green and warm earth gold",
     "the living spirit of the world, a serene goddess of earth and vine cradling "
     "a glowing green orb of raw primal nature power", 5334),
    # --- Sorcery ---
    ("spell_of_mastery", "Spell of Mastery",
     "brilliant white-blue and silver",
     "a luminous arcane crown of pure white-blue light hovering above a master "
     "archmage, the whole world bending to his will, supreme ultimate magic", 5341),
    ("detect_magic", "Detect Magic",
     "clear sapphire blue",
     "a glowing blue all-seeing crystal eye radiating concentric arcane rings "
     "that reveal hidden glowing sigils in the dark", 5342),
    ("diplomatic_scrying", "Diplomatic Scrying",
     "azure blue and silver",
     "a scrying crystal orb revealing distant rival courts and councils, fine "
     "threads of blue light connecting far-off thrones", 5343),
    ("suppress_magic", "Suppress Magic",
     "deep blue and muted silver",
     "a great blue rune-lock sealing a web of magical energy, swirling spells "
     "dampened into frozen stillness around it", 5344),
    ("aether_binding", "Aether Binding",
     "luminous blue and violet",
     "shimmering chains of blue aether light binding floating magical ley-line "
     "nodes together into a constellation", 5345),
    # --- Arcane ---
    ("spell_of_return", "Spell of Return",
     "silver-white and pale gold",
     "a wizard's spirit reforming from streams of silver light beside a glowing "
     "portal and a turning hourglass, resurrection and return", 5351),
    # --- Spirit ---
    ("dream_vision", "Dream Vision",
     "pale gold and dreamy white-violet",
     "a sleeping seer surrounded by floating dreamlike visions and pale golden "
     "spirit-wisps, ethereal prophetic foresight", 5361),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH,
        torch_dtype=torch.float16,
        use_safetensors=True,
        variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print(f"Pipeline ready. Generating {len(CARDS)} enchantment cards.\n")

    for slug, display, realm_color, centerpiece, seed in CARDS:
        out_path = os.path.join(OUT_DIR, f"enchant_{slug}.png")
        if os.path.exists(out_path):
            print(f"[{slug}] already exists, skipping")
            continue
        prompt = (
            f"\"{display}\", {centerpiece}, {realm_color} color palette, {STYLE}"
        )
        print(f"[{slug}] '{display}'  seed={seed}  {W}x{H}  steps={STEPS}")
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(
            prompt=prompt,
            negative_prompt=NEG,
            width=W, height=H,
            num_inference_steps=STEPS,
            guidance_scale=CFG,
            generator=gen,
        )
        result.images[0].save(out_path)
        kb = os.path.getsize(out_path) // 1024
        print(f"     -> {out_path}  ({kb} KB)")

    print(f"\nAll enchantment cards generated in {OUT_DIR}")


if __name__ == "__main__":
    main()
