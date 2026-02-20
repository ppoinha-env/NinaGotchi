import asyncio
import edge_tts
import re
from pathlib import Path

# --- CONFIGURATION ---
VOICE = "en-US-AnaNeural" 
SPEED = "-15%"             
PITCH = "-15Hz"           
OUTPUT_FOLDER = "Sounds"

# The phrases you requested
PHRASES = [
    "Hello Nina! I am your personal robot",
    "Ohh no, its raining",
    "Ahhhhhhhh ! What is happening",
    "Awwwwwww! I love you Nina",
    "Yum, this is tasty"
]

def slugify(text):
    """Converts phrases into safe filenames (e.g., 'Hello Nina!' -> 'hello_nina')"""
    text = text.lower()
    return re.sub(r'[^a-z0-9]+', '_', text).strip('_')

async def generate_phrases():
    # 1. Create the folder
    output_path = Path(OUTPUT_FOLDER)
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"Generating robotic phrases in: {output_path.resolve()}")

    # 2. Loop through the phrases
    for phrase in PHRASES:
        # Create a clean filename from the phrase
        filename = f"{slugify(phrase)}.mp3"
        file_dest = output_path / filename
        
        # 3. Use Edge-TTS to communicate with the neural engine
        communicate = edge_tts.Communicate(phrase, VOICE, rate=SPEED, pitch=PITCH)
        
        try:
            await communicate.save(str(file_dest))
            print(f"  [✓] Created: {filename}")
        except Exception as e:
            print(f"  [X] Failed '{phrase[:20]}...': {e}")

if __name__ == "__main__":
    asyncio.run(generate_phrases())
    print("\nProcess finished! Your robot is ready to talk.")