import os
import glob
import pathlib
import sys 

p = pathlib.Path(__file__).parent.joinpath("**/Dockerfile*")
for f in glob.glob(str(p)):
    print(f)
    print(os.system(f"cd {pathlib.Path(f).parent} && docker build -f {f} ." ))