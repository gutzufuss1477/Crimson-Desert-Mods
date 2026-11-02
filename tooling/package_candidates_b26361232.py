"""Package locally validated candidates; no gameplay claim and no publication."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile
from crimson_mod_tools.package_healthbar_204 import VARIANTS, UI_SHA
from crimson_mod_tools.staticinfo import Table

ROOT=Path(__file__).resolve().parents[1]
BUILD='26361232'
def sha(data):return hashlib.sha256(data).hexdigest()
def pack(path,files):
    assert len(files)==len(set(files))
    assert all(not n.startswith(('/', '\\')) and '..' not in Path(n).parts for n in files)
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(files.items()):
            info=zipfile.ZipInfo(name,(2026,11,2,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,data,compresslevel=9)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None and set(z.namelist())==set(files)
        assert all(z.read(n)==data for n,data in files.items())

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--healthbar-data',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    approved=json.loads((ROOT/'recipes/b26361232/native-candidates.json').read_text())
    assert approved['static_and_host_tests_passed'] and not approved['gameplay_tested']
    binaries={}
    for name,row in approved['binaries'].items():
        data=(ROOT/row['path']).read_bytes()
        assert sha(data)==row['sha256'],f'Native binary changed after validation: {name}'
        binaries[name]=data
    records=[]
    for variant,(label,description,needs_native) in VARIANTS.items():
        slug=f'Healthbar_always_on_{variant}_b{BUILD}'
        with zipfile.ZipFile(args.healthbar_data/(slug+'.zip')) as z:
            assert z.testzip() is None
            assert all(n.startswith(slug+'/') for n in z.namelist())
            files={n[len(slug)+1:]:z.read(n) for n in z.namelist() if not n.endswith('/')}
        for extension,digest in zip(('html','css'),UI_SHA[variant],strict=True):
            assert sha(files[f'files/0012/ui/xml/gamemain/play/subtitletagview.{extension}'])==digest
        report=json.loads(files['BUILD_VALIDATION.txt']);assert report['steam_build']==BUILD
        stem='files/0008/gamedata/binarystaticinfo__/bin/skill.'
        table=Table(files[stem+'staticinfobody'],files[stem+'staticinfoheader'])
        table.require(1000062,'Passive_Immune_BuffedActionRestriction');assert 1000061 not in table.by_key
        manifest=json.loads(files[slug+'.json']);manifest['name']='Healthbar - '+label
        manifest['version']='4.0-test';manifest['description']=description+' Candidate for Steam build '+BUILD+'.'
        files[slug+'.json']=(json.dumps(manifest,indent=2)+'\n').encode()
        helper='Enable the included Healthbar Multitarget plugin under ASI Mods as well.\n' if needs_native else 'Classic Single Target needs no Healthbar ASI helper.\n'
        files['README.txt']=(f'HEALTHBAR - {label}\nCandidate 4.0-test | Steam build {BUILD}\n\n{description}\n\n'
            'Close the game. In DMM unmount and remove/disable the previous Healthbar version.\n'
            'Import this complete ZIP and enable its data mod. '+helper+
            'Mount and restart the game. Choose exactly ONE Healthbar variant.\n'
            'Remove older manual NativeFix/Healthbar helper copies before using this package.\n'
            'Keep the existing ASI loader and DmmRuntime.asi. No separate helper download.\n\n'
            'Static, host and package checks passed; gameplay and DMM import of this candidate are pending.\n'
            'The helper resolves validated code and class identities on compatible layouts; ambiguous or\n'
            'changed structures are refused. Data overlays still target the listed Steam build.\n'
            'The three distinct UI styles are retained. Bars should disappear when enemies die.\n\n'
            'DE: Spiel schliessen, unmounten, alte Healthbar-Version entfernen/deaktivieren.\n'
            'Dieses ZIP importieren; Datenmod und bei Multitarget den ASI-Helfer aktivieren.\n'
            'Nur EINE Variante verwenden, mounten und Spiel neu starten. Spieltest steht noch aus.\n\n'
            'Credits: Blablup; Caites for Persistent HP Bars and the compact presentation.\n').encode()
        if needs_native:
            files['Healthbar Multitarget.asi']=binaries['healthbar']
            files['NATIVE_LICENSE.txt']=(ROOT/'mods/healthbar-always-on/source/native/LICENSE.txt').read_bytes()
        assert sum(n.endswith('.json') for n in files)==1
        assert sum(n.endswith('.asi') for n in files)==int(needs_native)
        destination=args.output/(slug+'.zip');pack(destination,files)
        records.append(dict(file=destination.name,sha256=sha(destination.read_bytes()),native=needs_native,kind='healthbar'))
    for name in ('mining','bank','allstats','speed'):
        row=approved['binaries'][name];filename=Path(row['path']).name
        instructions=(f"{row['label']}\nCandidate {row['version']} | Steam build {BUILD}\n\n"
            'Close the game. Remove older ASIs for this same mod, including manager-installed duplicates.\n'
            f'Copy {filename} into Crimson Desert/bin64 beside CrimsonDesert.exe.\n'
            'Keep your existing ASI loader and your existing INI settings. The loader is not bundled.\n'
            'DMM is optional for this standalone ASI; its ASI import can manage this ZIP.\n'
            'Use AllStats OR Speed-only, never both. Steelheart is optional and unchanged.\n\n'
            'Code fingerprints, structural references, host tests and packaging passed.\n'
            'Gameplay on this build remains untested. Incompatible/ambiguous structures are refused.\n'
            'No unconditional support for future game versions is claimed.\n\n'
            'DE: Alte ASI desselben Mods entfernen, neue ASI in bin64 kopieren.\n'
            'Vorhandene INI-Einstellungen behalten. AllStats ODER Speed-only verwenden.\n')
        files={filename:binaries[name],'README.txt':instructions.encode()}
        for source,target in row.get('extras',[]):files[target]=(ROOT/source).read_bytes()
        destination=args.output/f"{Path(filename).stem}_v{row['version']}_b{BUILD}.zip";pack(destination,files)
        records.append(dict(file=destination.name,sha256=sha(destination.read_bytes()),native=True,kind=name))
    (args.output/'SHA256SUMS.txt').write_text(''.join(f"{r['sha256']}  {r['file']}\n" for r in records))
    (args.output/'VALIDATION.json').write_text(json.dumps(dict(steam_build=BUILD,static_and_host_tests_passed=True,
        gameplay_tested=False,dmm_import_tested=False,files=records),indent=2)+'\n')
    print('PASS: seven candidate ZIPs; stable ASI names; three distinct complete Healthbar variants')

if __name__=='__main__':main()
