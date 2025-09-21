import argparse

def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Build the pdf and epub files of the Vulkan Tutorial."
    )

    parser.add_argument(
        "--geometry:left",
        type=str,
        required=False,
        default="2.5cm",
        help="Specify left margin space as a string. Example: 2cm.",
    )
    parser.add_argument(
        "--geometry:right",
        type=str,
        required=False,
        default="2.5cm",
        help="Specify right margin space as a string. Example: 2cm.",
    )
    parser.add_argument(
        "--geometry:top",
        type=str,
        required=False,
        default="2.5cm",
        help="Specify top margin space as a string. Example: 2cm.",
    )
    parser.add_argument(
        "--geometry:bottom",
        type=str,
        required=False,
        default="2.5cm",
        help="Specify bottom margin space as a string. Example: 2cm.",
    )

    return parser
