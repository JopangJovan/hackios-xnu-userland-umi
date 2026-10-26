# UMI userland source mirror

Upstream now keeps kernel and userland together. The complete UMI build and boot
workflow is maintained in [hackios-xnu-kernel-umi](https://github.com/JopangJovan/hackios-xnu-kernel-umi/tree/main/devices/xiaomi-mi-10).

This companion repository mirrors the unified upstream `userland/` source and
the UMI-only overlays in `devices/xiaomi-mi-10/overlays/userland/`. The original
userland templates stay unchanged; the main repository stages the UMI overlays
into its generated build workspace using `build-userland.sh`.

The unified upstream source is pinned to `88513176b1e96ecf0b764cd62c6f4e6ac71345e3`.
UMI retains its firmware packaging and native QCA6390 Bluetooth compatibility.
Vendor firmware and iOS files are supplied locally and are not included here.

Build and test from the main repository, which supplies the matching kernel,
loader, Mesa build, image tools, SDK setup and device scripts.
