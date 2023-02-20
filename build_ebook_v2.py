# """Generates epub and pdf from sources."""

import pathlib
import re
import shutil
import subprocess


class VTLogger:
    """A logger"""
    def __init__(self, filename:str, log_to_file:bool=True) -> None:
        if log_to_file is True:
            self.log_file = open(filename, "w", encoding="utf-8")

    def __del__(self) -> None:
        if self.log_file is not None:
            self.log_file.close()

    def detail(self, message: str) -> None:
        """Logs a detail message without printing it."""
        self._log(message, True)

    def error(self, message: str) -> None:
        """Logs an error."""
        message = f"Error: {message}"
        self._log(message)

    def info(self, message: str) -> None:
        """Logs an info message."""
        print(message)
        self._log(f"Info: {message}", True)

    def warning(self, message: str) -> None:
        """Logs an warning."""
        message = f"Warning: {message}"
        self._log(message)

    def _log(self, message: str, no_print:bool=False) -> None:
        if no_print is False:
            print(message)

        if self.log_file is not None:
            self.log_file.write(f"{message}\n")

class VTEMarkdownFile:
    """Markdown file."""

    def __init__(self, content: str, depth: int, prefix: str, title: str) -> None:
        self.content: str = content
        self.depth: str = depth
        self.prefix: str = prefix
        self.title: str = title

    def __repr__(self) -> str:
        return (
            f"<VTEMarkdownFile depth: {self.depth}, prefix: '{self.prefix}',"
            f" title: '{self.title}', content: '{self.content}'>"
        )

    def __str__(self) -> str:
        return (
            f"<VTEMarkdownFile depth: {self.depth}, prefix: '{self.prefix}',"
            f" title: '{self.title}', content: '{self.content}'>"
        )

class VTEBookBuilder:
    """A 'Markdown' to 'epub' and 'pdf' converter."""

    def __init__(self, logger: VTLogger) -> None:
        self.log = logger

    def build_pdf_book(self, language: str, markdown_filepath: pathlib.Path) -> None:
        """Builds a pdf file"""

        self.log.info("Building 'pdf'...")

        try:
            subprocess.check_output(
                [
                    "xelatex",
                    "--version",
                ]
            )
        except subprocess.CalledProcessError as error:
            log.error(error)
            self.log.warning("Please, install 'xelatex'!")

            raise RuntimeError from error

        try:
            subprocess.check_output(
                [
                    "pandoc",
                    markdown_filepath.as_posix(),
                    "-V", "documentclass=report",
                    "-t", "latex",
                    "-s",
                    "--toc",
                    "--listings",
                    "-H", "./ebook/listings-setup.tex",
                    "-o", f"./ebook/Vulkan Tutorial {language}.pdf",
                    "--pdf-engine=xelatex",
                    "--metadata=title:Vulkan Tutorial"
                ]
            )
        except subprocess.CalledProcessError as error:
            log.error(error)

            raise RuntimeError from error

    def build_epub_book(self, language: str, markdown_filepath: pathlib.Path) -> None:
        """Buids a epub file"""

        self.log.info("Building 'epub'...")

        try:
            subprocess.check_output(
                [
                    "pandoc",
                    markdown_filepath.as_posix(),
                    "--toc",
                    "-o", f"./ebook/Vulkan Tutorial {language}.epub",
                    "--epub-cover-image=ebook/cover.png",
                    "--metadata=title:Vulkan Tutorial"
                ]
            )
        except subprocess.CalledProcessError as error:
            log.error(error)

            raise RuntimeError from error

    def convert_svg_to_png(self, images_folder: str) -> list[pathlib.Path]:
        """Converts *.svg images to *.png using Inkscape"""

        self.log.info("Converting 'svg' images...")

        pngs = list[pathlib.Path]()

        for entry in pathlib.Path(images_folder).iterdir():
            if entry.suffix == ".svg":
                new_path = entry.with_suffix(".png")

                try:
                    subprocess.check_output(
                        [
                            "inkscape",
                            f"--export-filename={new_path.as_posix()}",
                            entry.as_posix()
                        ],
                        stderr=subprocess.STDOUT
                    )

                    pngs.append(new_path)
                except FileNotFoundError as error:
                    self.log.error(error)
                    self.log.warning("Install 'Inkscape' (https://www.inkscape.org)!")

                    raise RuntimeError from error

        return pngs

    def generate_joined_markdown(self, language: str, output_filename: pathlib.Path) -> None:
        """Processes the markdown sources and produces a joined file."""

        self.log.info(
            f"Generating a temporary 'Markdown' file: '{output_filename}'" \
            f" for language '{language}'..."
        )

        md_files = self._collect_markdown_files_from_source(language)
        md_files = sorted(md_files, key=lambda file: file.prefix)

        temp_markdown: str = str()

        for entry in md_files:
            # Add title.
            content: str = '# ' + entry.title + '\n\n' + entry.content

            # Fix image links.
            content = re.sub(r'\/images\/', 'images/', content)
            content = re.sub(r'\.svg', '.png', content)

            # Fix remaining relative links (e.g. code files).
            content = re.sub(r'\]\(\/', '](https://vulkan-tutorial.com/', content)

            # Fix chapter references
            def repl(match):
                target = match.group(1)
                target = target.lower()
                target = re.sub('_', '-', target)
                target = target.split('/')[-1]

                return '](#' + target + ')'

            content = re.sub(r'\]\(!([^)]+)\)', repl, content)

            temp_markdown += content + '\n\n'

        log.info("Writing markdown file...")

        with open(output_filename, "w", encoding="utf-8") as file:
            file.write(temp_markdown)

    def _collect_markdown_files_from_source(
        self,
        directory_path: pathlib.Path,
        current_depth: int=int(0),
        parent_prefix: str=str(),
        markdown_files: list[VTEMarkdownFile]=None
    ) -> list[VTEMarkdownFile]:
        """Traverses the directory tree, processes `Markdown` files."""
        if markdown_files is None:
            markdown_files = list[VTEMarkdownFile]()

        for entry in pathlib.Path(directory_path).iterdir():
            title_tokens = entry.stem.replace('_', ' ').split(" ")
            prefix = f"{parent_prefix}{title_tokens[0]}."

            if entry.is_dir() is True:
                log.info(f"Processing directory: {entry}")

                self._collect_markdown_files_from_source(
                    entry,
                    (current_depth + 1),
                    prefix, markdown_files
                )
            else:
                log.info(f"Processing: {entry}")

                title = ' '.join(title_tokens[1:])

                with open(entry, 'r', encoding="utf-8") as file:
                    content = file.read()
                    markdown_files.append(VTEMarkdownFile(content, current_depth, prefix, title))

        return markdown_files


###############################################################################


if __name__ == "__main__":

    out_dir = pathlib.Path("./_out")
    if not out_dir.exists():
        out_dir.mkdir()

    log = VTLogger(f"{out_dir.as_posix()}/build_ebook.log")
    eBookBuilder = VTEBookBuilder(log)

    log.info("--- Exporting ebooks:")

    generated_pngs = eBookBuilder.convert_svg_to_png("./images")

    LANGUAGES = [ "en", "fr" ]
    OUTPUT_MARKDOWN_FILEPATH = pathlib.Path(f"{out_dir.as_posix()}/temp_ebook.md")

    for lang in LANGUAGES:
        eBookBuilder.generate_joined_markdown(f"./{lang}", OUTPUT_MARKDOWN_FILEPATH)

        try:
            eBookBuilder.build_epub_book(lang, OUTPUT_MARKDOWN_FILEPATH)
            eBookBuilder.build_pdf_book(lang, OUTPUT_MARKDOWN_FILEPATH)
        except RuntimeError as runtimeError:
            log.error("Termininating...")

        # Clean up
        if OUTPUT_MARKDOWN_FILEPATH.exists():
            OUTPUT_MARKDOWN_FILEPATH.unlink()

    log.info("Cleaning up...")

    # Clean up temporary files
    for png_path in generated_pngs:
        try:
            png_path.unlink()
        except FileNotFoundError as fileError:
            log.error(fileError)

    # Comment to view log
    if out_dir.exists():
        shutil.rmtree(out_dir)

    log.info("---- DONE!")
