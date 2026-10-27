#!/usr/bin/env python3
"""Install the local Nexus port while GW2 is closed, preserving existing addons."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import sys

base = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--game', type=Path, default=Path.home()/'.local/share/Steam/steamapps/common/Guild Wars 2')
args = parser.parse_args()
for proc in Path('/proc').iterdir():
    if not proc.name.isdigit():
        continue
    try:
        if (proc/'comm').read_text().strip().lower() == 'gw2-64.exe':
            sys.exit('Close GW2 before installation so Nexus cannot overwrite the configuration.')
    except (OSError, ProcessLookupError):
        pass
addons = args.game/'addons'
config = addons/'Nexus/AddonConfig.json'
if not config.is_file() or not (args.game/'d3d11.dll').is_file():
    sys.exit('Nexus installation was not found at the selected game path.')
binary = base/'dist/InquestChipset.dll'
if not binary.is_file():
    sys.exit('Build dist/InquestChipset.dll first.')
records = json.loads(config.read_text())
if not isinstance(records,list):
    sys.exit('Unexpected Nexus configuration format.')
signature = -1905210500
matches = [r for r in records if r.get('Signature') in (signature,signature & 0xffffffff)]
if any(r.get('Name') != 'Inquest Chipset' for r in matches):
    sys.exit('The chosen addon signature is already used by another addon.')
stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
backup = config.with_name(config.name+'.before-inquest-'+stamp+'.bak')
shutil.copy2(config,backup)
target = addons/'InquestChipset.dll'
if target.exists():
    shutil.copy2(target,target.with_name(target.name+'.before-'+stamp+'.bak'))
assets = addons/'InquestChipset'
assets.mkdir(exist_ok=True)
shutil.copy2(base/'assets/double-click.wav',assets/'double-click.wav')
# Stage the DLL under an extension that Nexus will not attempt to load.
staged = target.with_suffix('.installing')
shutil.copy2(binary,staged)
staged.replace(target)
if matches:
    record = matches[0]
else:
    record = {'Signature':signature,'Name':'Inquest Chipset','IsFavorite':False,'AllowPrereleases':False,'IsPausingUpdates':False,'IsDisabledUntilUpdate':False}
    records.append(record)
record['IsLoaded'] = True
record['IsDisabledUntilUpdate'] = False
staged_config = config.with_suffix('.installing')
staged_config.write_text(json.dumps(records,indent=2)+'\n')
staged_config.replace(config)
assert hashlib.sha256(target.read_bytes()).digest()==hashlib.sha256(binary.read_bytes()).digest()
print('Installed:',target)
print('Automatic loading: enabled (IsLoaded=true)')
print('Configuration backup:',backup)
