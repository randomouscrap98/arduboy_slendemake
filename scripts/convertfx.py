import arduboy.fxdata_build
import os

# This one is super simple, just run the function
print("Building fx data...")
writefiles = arduboy.fxdata_build.build_fx(os.path.join("..", "fx", "fxdata.txt"))

print("Wrote files:")
for k,wf in writefiles.items():
    print(f"{k} - {wf}")

print("Done!")
