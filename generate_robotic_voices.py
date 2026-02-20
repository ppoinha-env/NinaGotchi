import asyncio
import edge_tts
import os
from pathlib import Path

# --- CONFIGURATION ---
VOICE = "en-US-AnaNeural"  # A clear, friendly male voice
SPEED = "-15%"              # Can be "-25%" to make it slower for kids
PITCH = "-15Hz"            # Lowering pitch makes it sound more "robotic"
OUTPUT_FOLDER = "Sounds"

async def generate_alphabet():
    # 1. Create the folder
    output_path = Path(OUTPUT_FOLDER)
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"Generating robotic alphabet in: {output_path.resolve()}")

    # 2. Loop through letters
    for char_code in range(ord('A'), ord('Z') + 1):
        letter = chr(char_code)
        filename = f"{letter.lower()}.mp3"
        file_dest = output_path / filename
        
        # We wrap the letter in a slight pause for clarity
        text = f"{letter}." 
        
        # 3. Use Edge-TTS to communicate with the neural engine
        communicate = edge_tts.Communicate(text, VOICE, rate=SPEED, pitch=PITCH)
        
        try:
            await communicate.save(str(file_dest))
            print(f"  [✓] Created: {filename}")
        except Exception as e:
            print(f"  [X] Failed {letter}: {e}")

if __name__ == "__main__":
    # Run the asynchronous loop
    asyncio.run(generate_alphabet())
    print("\nProcess finished! Check the Sounds folder.")