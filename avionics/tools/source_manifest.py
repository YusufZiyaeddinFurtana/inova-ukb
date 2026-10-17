"""Hash the complete source/dependency set shared by recorded test runs."""
import hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def source_hashes():
    result={}
    for folder in ('core','firmware','sim','tools','tests','third_party'):
        for path in sorted((ROOT/folder).rglob('*')):
            if path.is_file() and path.suffix in ('.c','.h','.hpp','.cpp','.py','.java','.json'):
                result[str(path.relative_to(ROOT))]=hashlib.sha256(path.read_bytes()).hexdigest()
    for name in ('Makefile','platformio.ini','requirements.txt'):
        result[name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    return result
