import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_source_map_entities import audit_bytes,extract_entity_lump,parse_entity_text,MapFormatError

ENT=b'''// small synthetic test; not copied from the game
{
"classname" "worldspawn"
}
{
"classname" "logic_auto"
"OnMapSpawn" "relay,Trigger,,0,-1"
"OnMapSpawn" "missing,Trigger,,0,-1"
"OnMapSpawn" "!activator,Fire,,0,-1"
}
{
"classname" "logic_relay"
"targetname" "relay"
"OnTrigger" "door\\x1bOpen\\x1b\\x1b0\\x1b1"
}
{
"classname" "func_door"
"targetname" "door"
}
{
"classname" "info_landmark"
"targetname" "landmark_a"
}
{
"classname" "trigger_changelevel"
"map" "se1_nextlevel"
"landmark" "landmark_a"
}
'''.replace(b'\\x1b',bytes([27]))

def make_bsp(data=ENT, version=20):
    out=bytearray(8+64*16+4)
    out[:4]=b'VBSP';struct.pack_into('<i',out,4,version)
    struct.pack_into('<IIII',out,8,len(out),len(data),0,0)
    return bytes(out)+data+b'other fake synthetic BSP bytes'

class SourceMapTests(unittest.TestCase):
    def test_lump_and_map_inventory(self):
        for name,data in [('synthetic.ent',ENT),('synthetic.bsp',make_bsp())]:
            r=audit_bytes(data,name)
            self.assertEqual(r['entity_count'],6)
            self.assertEqual(r['output_count'],4)
            self.assertEqual(r['resolved_unique_output_links'],2)
            self.assertEqual(len(r['unresolved_literal_outputs']),1)
            self.assertEqual(len(r['dynamic_or_wildcard_outputs']),1)
            self.assertEqual(len(r['unresolved_local_landmark_names']),0)
            self.assertEqual(r['changelevel_references'][0]['map'],'se1_nextlevel')
            self.assertIn('worldspawn',r['classnames_not_in_selected_32_factory_subset'])
            self.assertEqual(r['selected_original_registered_subset']['logic_auto'],1)
            self.assertNotIn('entity_data',r)

    def test_duplicate_targets_and_bad_output(self):
        data=b'{"classname" "logic_relay" "OnTrigger" "one,Fire,,0,1" "OnTrigger" "badformat"}\n'+b'{"classname" "func_button" "targetname" "one"}\n'*2
        r=audit_bytes(data,'test.ent')
        self.assertEqual(len(r['ambiguous_duplicate_target_outputs']),1)
        self.assertEqual(len(r['malformed_output_shapes']),1)

    def test_wildcards_are_dynamic(self):
        data=b'{"classname" "logic_relay" "OnTrigger" "door*,Open,,0,1"}'
        r=audit_bytes(data,'test.ent')
        self.assertEqual(len(r['dynamic_or_wildcard_outputs']),1)
        self.assertEqual(len(r['unresolved_literal_outputs']),0)

    def test_changelevel_landmark_missing(self):
        data=b'{"classname" "trigger_changelevel" "map" "a" "landmark" "not_here"}'
        r=audit_bytes(data,'test.ent')
        self.assertEqual(len(r['unresolved_local_landmark_names']),1)

    def test_unterminated_quote(self):
        with self.assertRaises(MapFormatError):parse_entity_text(b'{"classname" "logic_auto}')

    def test_unquoted_field(self):
        with self.assertRaises(MapFormatError):parse_entity_text(b'{foo "bar"}')

    def test_bad_bsp_lump_bounds(self):
        invalid=bytearray(make_bsp());struct.pack_into('<I',invalid,8,0xFFFFFF00)
        with self.assertRaises(MapFormatError):audit_bytes(bytes(invalid),'level.bsp')

    def test_compressed_bsp_refused(self):
        invalid=bytearray(make_bsp());struct.pack_into('<I',invalid,20,123)
        with self.assertRaises(MapFormatError):audit_bytes(bytes(invalid),'level.bsp')

    def test_unrecognized_bsp_refused(self):
        with self.assertRaises(MapFormatError):audit_bytes(make_bsp(version=999),'level.bsp')
        with self.assertRaises(MapFormatError):audit_bytes(b'BSP\0'+bytes(2000),'level.bsp')

    def test_text_limits(self):
        from audit_source_map_entities import MAX_ENTITY_BYTES
        with self.assertRaises(MapFormatError):extract_entity_lump(b' '*(MAX_ENTITY_BYTES+1),'a.ent')

    def test_comment_and_escaped_quotes(self):
        data=b'// hi\n{"classname" "logic_relay" "targetname" "a\\\"b"} // trail'
        r=audit_bytes(data,'test.ent')
        self.assertEqual(r['targetname_count'],1)

if __name__=='__main__':unittest.main()
