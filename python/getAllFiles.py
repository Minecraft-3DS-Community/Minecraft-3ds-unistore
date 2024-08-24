import os, sys
import requests
from bs4 import BeautifulSoup

url = "https://github.com/Minecraft-3DS-Community/Minecraft-3ds-unistore/tree/main/Sprites"
cwd = os.getcwd()
output_file = os.path.join(cwd, 'file.t3s')

response = requests.get(url)
soup = BeautifulSoup(response.text, 'html.parser')
png_files = set()

for link in soup.find_all('a'):
    href = link.get('href')
    if href and href.endswith('.png'):
        filename = href.split('/')[-1]
        if filename[:-4].isdigit():
            png_files.add(filename)
            download_url = f"https://raw.githubusercontent.com{href.replace('/blob', '')}"
            response = requests.get(download_url)
            print(f"Downloading File: {filename}")
            with open(os.path.join(cwd, filename), 'wb') as f:
                f.write(response.content)

if '0.png' not in png_files:
    sys.exit(1)

sorted_png_files = sorted(png_files, key=lambda x: int(x[:-4]))
with open(output_file, 'w') as f:
    f.write('--atlas -f rgba -z auto\n\n')
    for png in sorted_png_files:
        f.write(f"{png}\n")

print(f"Downloaded {len(png_files)} files and wrote the list to file.t3s.")
if os.path.exists('.\\mc3ds.t3x'):
    os.remove(".\\mc3ds.t3x")

os.system('tex3ds -i file.t3s -o mc3ds.t3x')
