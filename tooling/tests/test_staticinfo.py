import struct
import unittest

from crimson_mod_tools.staticinfo import Table, unique_offset


def sample(key_size=4):
    names = ((7, "Original", b"payload"), (19, "New_DLC_Record", b"\xff\x00\xfe"))
    body, header = bytearray(), bytearray(struct.pack("<H", len(names)))
    for key, name, payload in names:
        header += key.to_bytes(key_size, "little") + struct.pack("<I", len(body))
        body += struct.pack("<I", len(name)) + name.encode() + key.to_bytes(key_size, "little") + payload
    return bytes(body), bytes(header)


class TableTests(unittest.TestCase):
    def test_replacement_preserves_new_records_and_rebuilds_offsets(self):
        for key_size in (2, 4):
            table = Table(*sample(key_size), key_size)
            target = table.require(7, "Original")
            changed = table.replace({7: target.data[:target.prefix_size] + b"longer modified payload"})
            self.assertEqual([r.key for r in changed.records], [7, 19])
            self.assertEqual(changed.by_key[19].data, table.by_key[19].data)
            self.assertEqual(changed.by_key[19].start, len(changed.by_key[7].data))
            same = table.replace({})
            self.assertEqual((same.body, same.header), sample(key_size))

    def test_malformed_index_or_record_is_rejected(self):
        body, header = sample()
        bad_headers = [header[:-1], header + b"\0", header[:6] + struct.pack("<I", 1) + header[10:],
                       header[:10] + header[2:6] + header[14:], header[:14] + struct.pack("<I", len(body) + 1)]
        for bad in bad_headers:
            with self.assertRaises(ValueError):
                Table(body, bad)
        # Opaque payload integrity is checked by the builder's input SHA gate.
        for bad in (struct.pack("<I", 0xffffffff) + body[4:], body[:12] + b"\x08" + body[13:]):
            with self.assertRaises(ValueError):
                Table(bad, header)

    def test_identity_change_and_missing_target_are_rejected(self):
        table = Table(*sample())
        with self.assertRaises(ValueError):
            table.replace({99: b"unknown"})
        with self.assertRaises(ValueError):
            table.replace({7: table.by_key[19].data})
        with self.assertRaises(ValueError):
            table.require(7, "Wrong_Name")

    def test_patch_context_must_be_unique(self):
        self.assertEqual(unique_offset(b"abc123def", b"123"), 3)
        for data in (b"abc", b"123abc123"):
            with self.assertRaises(ValueError):
                unique_offset(data, b"123")


if __name__ == "__main__":
    unittest.main()
