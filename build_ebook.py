import subprocess
import datetime
import os
import re

# Recursively gather all markdown files in the right order
markdownFiles = []

for root, subdirs, files in os.walk('.'):
    for fn in files:
        if 'md' in fn and 'ebook.md' not in fn:
            path = os.path.join(root, fn)

            # "02_Development_environment.md" -> "Development environment"
            title = fn.split('.')[0] # "02_Development_environment.md" -> "02_Development_environment"
            title = title.replace('_', ' ') # "02_Development_environment" -> "02 Development environment"
            title = ' '.join(title.split(' ')[1:]) # "02 Development environment" -> "Development environment"

            with open(path, 'r') as f:
                markdownFiles.append({
                    'title': title,
                    'filename': os.path.join(root, fn),
                    'contents': f.read()
                })

markdownFiles.sort(key=lambda entry: entry['filename'])

# Create concatenated document
print('processing markdown...')

allMarkdown = ''

for entry in markdownFiles:
    contents = entry['contents']

    # Add title
    contents = '# ' + entry['title'] + '\n\n' + contents

    # Fix image links
    contents = re.sub(r'\/images\/', 'images/', contents)
    contents = re.sub(r'\.svg', '.png', contents)

    # Fix remaining relative links (e.g. code files)
    contents = re.sub(r'\]\(\/', '](https://vulkan-tutorial.com/', contents)

    # Fix chapter references
    def repl(m):
        target = m.group(1)
        target = target.lower()
        target = re.sub('_', '-', target)
        target = target.split('/')[-1]

        return '](#' + target + ')'

    contents = re.sub(r'\]\(!([^)]+)\)', repl, contents)

    allMarkdown += contents + '\n\n'

# Add title
dateNow = datetime.datetime.now()

metadata = '% Vulkan Tutorial\n'
metadata += '% Alexander Overvoorde\n'
metadata += '% ' + dateNow.strftime('%B %Y') + '\n\n'

allMarkdown = metadata + allMarkdown

with open('ebook.md', 'w') as f:
    f.write(allMarkdown)

# Convert all SVG images to PNG for pandoc
print('converting svgs...')

generatedPngs = []

for fn in os.listdir('images'):
    parts = fn.split('.')

    if parts[1] == 'svg':
        subprocess.check_output(['inkscape', '-z', '-e', 'images/' + parts[0] + '.png', 'images/' + fn], stderr=subprocess.STDOUT)
        generatedPngs.append('images/' + parts[0] + '.png')

# Building PDF
print('building pdf...')

subprocess.check_output(['pandoc', 'ebook.md', '-V', 'documentclass=report', '-t', 'latex', '-s', '--toc', '-o', 'ebook/Vulkan Tutorial.pdf'])

print('building epub...')

subprocess.check_output(['pandoc', 'ebook.md', '--toc', '-o', 'ebook/Vulkan Tutorial.epub'])

# Clean up
os.remove('ebook.md')

for fn in generatedPngs:
    os.remove(fn)