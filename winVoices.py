import pyttsx3
engine = pyttsx3.init()
voices = engine.getProperty('voices')

for index, voice in enumerate(voices):
    print(f"Index: {index} | Name: {voice.name} | ID: {voice.id}")