import subprocess
import datetime
import os
import re


def create_ebook(path):

    name_path = path
    print('\n Creating \"' + name_path + '\" ebook')
    # Recursively gather all markdown files in the right order
    markdownFiles = []

    for root, subdirs, files in os.walk(name_path):
        for fn in files:
            if 'md' in fn and 'ebook.md' not in fn:
                path = os.path.join(root, fn)

                # "02_Development_environment.md" -> "Development environment"
                # "02_Development_environment.md" -> "02_Development_environment"
                title = fn.split('.')[0]
                # "02_Development_environment" -> "02 Development environment"
                title = title.replace('_', ' ')
                # "02 Development environment" -> "Development environment"
                title = ' '.join(title.split(' ')[1:])

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
        contents = re.sub(
            r'\]\(\/', '](https://vulkan-tutorial.com/', contents)

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

    # Building PDF
    print('building pdf...')

    subprocess.check_output(['pandoc', 'ebook.md', '-V', 'documentclass=report', '-t', 'latex', '-s',
                             '--toc', '--listings', '-H', 'ebook/listings-setup.tex', '-o', 'ebook/Vulkan Tutorial ' + name_path + '.pdf', '--pdf-engine=xelatex'])

    print('building epub...')

    subprocess.check_output(
        ['pandoc', 'ebook.md', '--toc', '-o', 'ebook/Vulkan Tutorial ' + name_path + '.epub', '--epub-cover-image=ebook/cover.png'])

    # Clean up
    os.remove('ebook.md')


# Convert all SVG images to PNG for pandoc
print('converting svgs...')

generatedPngs = []

for fn in os.listdir('images'):
    parts = fn.split('.')

    if parts[1] == 'svg':
        subprocess.check_output(['inkscape', '--export-filename=images/' +
                                 parts[0] + '.png', 'images/' + fn], stderr=subprocess.STDOUT)
        generatedPngs.append('images/' + parts[0] + '.png')

create_ebook('en')
create_ebook('fr')

for fn in generatedPngs:
    os.remove(fn)
