# Enclosure (v3, landscape)

`fusion_case_v3.py` is a Fusion 360 API script that builds the enclosure:
- Cover
- Backplate
- 5 D-pad keys
- switch carrier
- non-printed reference bodies

Run it in Fusion, either through the Fusion MCP or from Utilities → Scripts, in **an empty design document**. It deletes and rebuilds every component in the active document. It also refuses to run if the document contains anything it didn't create.

- **Size.** 138 × 114 × 27 mm (W × H × D). PETG. Print the cover face-down. Every printed part fits the Bambu A1 mini bed.
- **Layout.**
  - Left: the relay module, with the controller board below it.
  - Right: the OLED, the XL7015 underneath it, the D-pad, and the sensor chamber at the bottom right.
- **Wall mount.** Two #8 screws on horizontal slots at y = −13.5. The slots accept 65–85 mm spacing, to match the old Braeburn plate. The wall wire window is centred between them.
- **Inserts.** All M3×4 heat-set inserts. The cover is retained by snaps plus one M3 screw in the bottom wall.
- **Review.** See `../docs/reviews/v3_synthesis.md` for the rulings and the reasoning behind each dimension.

## Before printing

- Measure the wall screw spacing and the wire-hole size.
- Check whether the case covers the old paint outline. The case reaches 43.5 mm below the screw line.
- Measure the XL7015 size and trim-pot height. The rails assume about 49 × 26 mm and 12 mm tall.
- Measure the switch lever travel to the click. The design gives 1.5 mm of key travel and 0.2 mm of preload.
- The fuse holders are modelled as uxcell PCB clip holders on the controller board.
