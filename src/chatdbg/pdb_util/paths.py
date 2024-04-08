import os
import sys


def is_library_file(file_path):
    # Check if file is in standard library directory
    std_lib_path = os.path.dirname(os.__file__)
    if os.path.commonpath([file_path, std_lib_path]) == std_lib_path:
        return True
    # Check if file is in site-packages or dist-packages (common locations for installed libraries)
    for path in sys.path:
        if "site-packages" in path or "dist-packages" in path:
            if os.path.commonpath([file_path, path]) == path:
                return True
    # Assume the file is a user-written file if it doesn't match the above criteria
    return False


def main():
    user_path = os.path.abspath(sys.path[0])
    print(f"*** user path: {user_path} ***")

    library_paths = [os.path.dirname(os.__file__)] + [
        path for path in sys.path if "site-packages" in path or "dist-packages" in path
    ]

    print(library_paths)

    file_path = "/path/to/your/file.py"
    if is_library_file(file_path):
        print(f"{file_path} is likely a library file.")
    else:
        print(f"{file_path} is likely a user-written file.")


if __name__ == "__main__":
    print(f"std lib: {os.path.dirname(os.__file__)}")
    for path in sys.path:
        if "site-packages" in path or "dist-packages" in path:
            print(f"packages: {path}")

    main()
