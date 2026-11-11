"""Check production map declarations against independently captured live metadata."""
from pathlib import Path
import argparse
import json
import re


def check(metadata_path):
    metadata = json.loads(Path(metadata_path).read_text())
    assert metadata['GameHash'] == '14f711a6a3b1b7cd803a03f85cc6c6201c5b546ab9b90329533f5cb8ae2e7f13'
    classes = {(c['Namespace'], c['Name']): c for a in metadata['Assemblies'] for c in a['Classes']}
    source = (Path(__file__).resolve().parents[1] / 'mod.c').read_text()
    boundary = (Path(__file__).resolve().parents[1] / 'ground_pick.h').read_text()
    methods = re.findall(r'MM\((\w+),"([^"]*)","([^"]*)","([^"]*)","([^"]*)"\)', source)
    assert methods
    for slot, namespace, kind, name, signature in methods:
        matches = [m for m in classes[namespace, kind]['Methods'] if m['Name'] == name
                   and ';'.join(p['Type']['Name'] for p in m['Parameters']) == signature]
        assert len(matches) == 1, (slot, namespace, kind, name, signature, matches)
    inverse_methods = re.findall(r'GB\((\w+),"([^"]*)","([^"]*)","([^"]*)","([^"]*)",(\d+)\)', boundary)
    assert len(inverse_methods) == 3
    for slot, namespace, kind, name, signature, token in inverse_methods:
        matches = [m for m in classes[namespace, kind]['Methods'] if m['Name'] == name
                   and ';'.join(p['Type']['Name'] for p in m['Parameters']) == signature]
        assert len(matches) == 1 and matches[0]['Token'] == int(token), (slot, matches)
    project = Path(__file__).resolve().parents[3]
    joonas = (project / 'mods/joonas-map-commands/mod.c').read_text()
    for owner in (source, joonas):
        assert 'mmc_ground_pick_local(' in owner and 'mmc_ground_bind(' in owner
    assert 'call(mc.pick' not in source and '"MoonMapScreen","Pick","UnityEngine.Vector2"),screen' not in joonas
    map_class = classes['MoonCleaners', 'MoonMapScreen']
    fields = {f['Name']: f for f in map_class['Fields']}
    for slot, name, kind, offset in re.findall(r'MF\((\w+),"([^"]*)","([^"]*)",(\d+)\)', source):
        actual = fields[name]
        assert actual['Type']['Name'] == kind and actual['Offset'] == int(offset), (slot, actual)
    assert fields['overlayLayer']['Type']['Name'] == 'UnityEngine.RectTransform' and fields['overlayLayer']['Offset'] == 640
    tokens = {m['Name']: m['Token'] for m in map_class['Methods']}
    assert tokens['Steer'] == 100668206 and tokens['Pick'] == 100668218
    pick = next(m for m in map_class['Methods'] if m['Name'] == 'Pick')
    assert pick['ReturnType']['Name'] == 'System.Nullable<UnityEngine.Vector3>'
    names=re.search(r'const char \*screen_names\[\]=\{([^}]+)\}',source)
    assert names
    for kind in re.findall(r'"([^"]+)"',names[1]):
        matches=[m for m in classes['MoonCleaners',kind]['Methods'] if m['Name']=='get_IsOpen' and not m['Parameters']]
        assert len(matches)==1 and matches[0]['ReturnType']['Name']=='System.Boolean',kind
    print(f'{len(methods)} exact methods plus 3 inverse-boundary methods, overlay/map fields, native tokens and both production callsites match')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('metadata', type=Path)
    check(parser.parse_args().metadata)
