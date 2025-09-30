import os
import sys


def add_dll_search_paths():
    if sys.platform == "win32":
        dll_path_str = os.getenv("DLL_PATHS", "")
        dll_paths = [p for p in dll_path_str.split(";") if p]
        for p in dll_paths:
            os.add_dll_directory(p)
