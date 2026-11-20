from pathlib import Path
import os, shutil, subprocess
R=Path(__file__).resolve().parents[1]
os.chdir(R)
R=Path('.')
P=R/'firmware/encoder_test'; O=R/'build/host'; O.mkdir(parents=True,exist_ok=True)
gcc=os.environ.get('CC') or shutil.which('gcc')
if not gcc: raise SystemExit('GCC not found. Set CC or add gcc to PATH.')
exe=O/('test_ffb.exe' if os.name=='nt' else 'test_ffb')
subprocess.run([gcc,'-std=c11','-Wall','-Wextra','-Werror','-I'+str(P/'App/Inc'),str(P/'Tests/test_ffb.c'),*[str(p) for p in sorted((P/'App/Src').glob('*.c'))],'-lm','-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
