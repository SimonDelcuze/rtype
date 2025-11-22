# Networking

This section documents the server’s binary protocol building blocks used by the UDP networking pipeline.\
It defines the fixed-size packet header shared by all messages and explains the rules that guarantee cross-platform compatibility (Windows, Linux, macOS).

***

## **Contents**

- [Packet Header](packet-header.md)

***

## **Design Goals**

- Fixed, compact metadata for every message
- Deterministic, big-endian (network order) encoding
- Cross-platform and compiler-agnostic representation
- Simple encode/decode without dynamic allocation

