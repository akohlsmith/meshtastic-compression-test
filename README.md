# Meshtastic Compression Test

This is a *very* rough utility which attempts to gather real-world statistics on the compressibility of Meshtastic traffic using an arithmetic coder I found online and stripped down/cleaned up for embedded use.

The arithmetic coder is written by [Nathan Clack](https://github.com/nclack/arithcode); I do not propose to understand how it works, but he has documented his [source of inspiration](http://www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf) and documented his code in a way that I am sure mathematicians understand, but which I can not even begin to comprehend. Still, I was able to distill it down to a small pair of .c and .h files, and have successfully run it on embedded (STM32) platforms. There is still more work to be done to help document and test.

This utility will connect to an MQTT server and subscribe to a single topic. For each Meshtastic MQTT message received, it will extract the raw packet data and then attempt to decrypt it using the default key (`AQ==` which is a shorthand for the actual AES256-CTR key used by Meshtastic, which is `d4f1bb3a20290759f0bcffabcf4e6901`). If decryption is successful it will then compress and then decompress the packet, verify that the packet is unchanged and record some statistics. (**TODO:** If configured to do so, it will publish the statistics to another MQTT topic on the same server.) It will also periodically (every 1000 packets) print out the statistics it has gathered to stdout. The utility does attempt to de-duplicate the incoming MQTT traffic stream since my use case involves capturing traffic from as wide a net as possible, and a single packet being uplinked several times definitely happens. I didn't want them to skew the compression statistics.

**This is not production-quality code** -- there is no guarantee or other assurance that it won't blow up your computer, infect the internet with a terrible AI virus, leave the cap off your toothpaste or run off with your wife.

## Prerequisites

- C compiler
- GNU Make
- nanopb
- gnu-tls (if you wish to connect to MQTT servers using TLS)

## Getting Started

This project uses Git submodules. To clone the repository with all submodules, use:

```bash
git clone https://github.com/akohlsmith/meshtastic-compression-test.git
cd meshtastic-compression-test
git submodule update --init --recursive
```

Alternatively, you can clone with submodules in a single command:

```bash
git clone --recursive https://github.com/akohlsmith/meshtastic-compression-test.git
```

If you've already cloned the repository without the `--recursive` flag, you can pull the submodules later:

```bash
git submodule update --init --recursive
```

### Building the Project

This is pretty simple. It assumes that nanopb is installed at the default location (at least on OSX and probably Linux). This is not set up to use `pkg-config` although that is on the list.

```bash
make
```

### Running

This is currently a little ... messy. I'll add `optarg` style options soon (I hope)

```bash
./meshtastic-compression-test mqtt.server.name mqtt-port topic mqtt_username mqtt_password
```

e.g. to use the global Meshtastic MQTT server, subscribing to `msh/US/CA/socalmesh` and listening to LongFast traffic published by any MQTT gateway:

```
./meshtastic-compression-test mqtt.meshtastic.org 1883 msh/US/CA/socalmesh/2/e/LongFast/\# meshdev large4cats
```

### Statistics

Every time a message is successfully received, decoded, compressed and decompressed, a message is emitted to stdout:

```
 POSITION_APP: 30.00% (222 symbols: 20 -> 14 bytes) best: 20 -> 14, worst: 28 -> 19, avg 27.2 bytes, avg ratio 31.93% over 2 packets
TELEMETRY_APP: 26.09% (225 symbols: 23 -> 17 bytes) best: 22 -> 17, worst: 51 -> 38, avg 25.9 bytes, avg ratio 23.66% over 4 packets
 NODEINFO_APP: 34.09% (219 symbols: 44 -> 29 bytes) best: 44 -> 29, worst: 87 -> 66, avg 82.7 bytes, avg ratio 25.13% over 2 packets
```

Each line provides some statistics about the message itself, and all messages of that type which have been received to date:

* `POSITION_APP` - the type of message

* `30.00% (222 symbols: 20 -> 14 bytes)` - the compression ratio with CDR symbols and byte counts

* `best: 20 -> 14, worst: 28 -> 19` - best and worst compression achieved for this message type

* `avg 27.2 bytes, avg ratio 31.93%` - running averages of the uncompressed length and compression ratio for this type

* `over 2 packets` - how many packets of this type were analyzed so far

  

Every 1000 received packets, it will dump out a summary of all received packet types, like this:

```
COMPRESSION STATS (15000 packets total, 1000 in the last 45 minutes, 45 seconds):
        NODEINFO_APP: avg comp. ratio 25.21% over 6254 packets (370 in this interval), 37.0%/41.7% of all packets this interval/ever
       TELEMETRY_APP: avg comp. ratio 26.71% over 3913 packets (229 in this interval), 22.9%/26.1% of all packets this interval/ever
        POSITION_APP: avg comp. ratio 30.79% over 3718 packets (288 in this interval), 28.8%/24.8% of all packets this interval/ever
      TRACEROUTE_APP: avg comp. ratio 37.06% over 574 packets (35 in this interval), 3.5%/3.8% of all packets this interval/ever
         ROUTING_APP: avg comp. ratio 0.00% over 354 packets (68 in this interval), 6.8%/2.4% of all packets this interval/ever
    TEXT_MESSAGE_APP: avg comp. ratio 32.80% over 107 packets (3 in this interval), 0.3%/0.7% of all packets this interval/ever
    NEIGHBORINFO_APP: avg comp. ratio 31.92% over 43 packets (3 in this interval), 0.3%/0.3% of all packets this interval/ever
   STORE_FORWARD_APP: avg comp. ratio 0.00% over 35 packets (4 in this interval), 0.4%/0.2% of all packets this interval/ever
           ADMIN_APP: avg comp. ratio 0.00% over 1 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
        WAYPOINT_APP: avg comp. ratio 28.57% over 1 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
```

For each packet type you can see what the average compression ratio was, and over how many packets of that type this was computed. It also emits some statistics about what percentage of all traffic this specific packet type occupies, both in the last interval as well as for the lifetime that the utility was running.

## Why Arithmetic Coding

I began wondering about the compressibility of Meshtastic traffic when I started writing my own firmware for the communications system. Watching the data dumps scroll by I couldn't help noticing that there were a lot of repeated sequences and close-to-repeating sequences in the raw protobufs. Grabbing some traffic, I ran them through the usual suspects: zlib, gzip, bzip2, xz, and ever more esoteric compressors.

All of these failed to produce any kind of meaningful compression, and in fact most failed to compress the small binary fragments at all. Several compressors require large dictionaries to be generated and this was a non-starter for a speed- and space-constrained protocol like Meshtastic. I also tried simpler/faster coding methods such as RLE but they were all pretty poor performers.

I'd heard about [arithmetic coding](https://en.wikipedia.org/wiki/Arithmetic_coding) but didn't know much about it, and not being much of an academic, most of the information online was not very approachable to me. I somehow stumbled across [Nathan Clack](https://github.com/nclack/arithcode)'s arithcode repo and noticed right away that it was straight C (a hard requirement for me) and didn't appear to require a large amount of compute horsepower to compress or decompress. I found his code very difficult to understand, but set out simplifying and refactoring it into something I could understand and build for embedded systems. The code in the `arithcode/` directory is the (in progress) result of that.

### Notes

It appears that 20-40% compression is achievable for unencrypted Meshtastic protobufs. The average compression ratio bumps up ever so slightly (2-4%) if the Meshtastic radio header can also be compressed. This would break the existing mesh, but having a single bit to indicate "compressed packet" would allow both to function in a mixed mesh. The Meshtastic 3.0 protocol is slowly coming together, and I'm hopeful that releasing this code will allow others to test the compression in their own networks and hopefully provide enough evidence that this is something which should be part of the v3.0 protocol.

Compression and decompression is quite fast, even on modest hardware. I'm building for the STM32 running at 48MHz and the average packet can be compressed or decompressed in under 50ms, even with no code optimization compiler flags enabled. I'm absolutely certain that there are additional gains to be made through refactoring the code, but I'm just about at the limit of what I can personally do without a better understanding of what actually is going on in the arithmetic coder.

## Sample Corpus

I have included a large packet dump (over 200k LoRa packets) so people could test without needing an MQTT connection. It was generated by connecting to the global MQTT server and subscribing to `msh/+/2/e/LongFast/#`. Duplicate packets (i.e. a packet which was uplinked by more than one MQTT-connected client) have been stripped. Each line is just a dump of the entire packet (16 byte header + decrypted payload).

e.g.

```
38 14 48 43 ff ff ff ff de 0a 5a c6 60 08 00 38 08 03 12 16 0d 00 00 39 20 15 00 00 41 08 18 05 28 01 78 00 80 01 00 b8 01 0f 48 01
```

The uncompressed log is 39MB. The statistics I have from this dump are as follows:

```
COMPRESSION STATS (219000 packets total, 1000 in the last 5 minutes, 5 seconds):
       TELEMETRY_APP: min: 12 -> 10, max: 49 -> 37, avg unc. length 26.8 bytes, avg comp. ratio 26.83% over 70064 packets (340 in this interval), 34.0%/32.0% of all packets this interval/ever
        POSITION_APP: min: 8 -> 7, max: 55 -> 37, avg unc. length 29.8 bytes, avg comp. ratio 32.70% over 70639 packets (304 in this interval), 30.4%/32.3% of all packets this interval/ever
        NODEINFO_APP: min: 29 -> 20, max: 104 -> 82, avg unc. length 65.9 bytes, avg comp. ratio 23.36% over 69086 packets (272 in this interval), 27.2%/31.5% of all packets this interval/ever
         ROUTING_APP: min: 8 -> 7, max: 8 -> 7, avg unc. length 4.6 bytes, avg comp. ratio 7.12% over 7697 packets (65 in this interval), 6.5%/3.5% of all packets this interval/ever
    TEXT_MESSAGE_APP: min: 6 -> 5, max: 200 -> 119, avg unc. length 43.5 bytes, avg comp. ratio 34.84% over 1441 packets (18 in this interval), 1.8%/0.7% of all packets this interval/ever
           ADMIN_APP: min: 13 -> 11, max: 13 -> 11, avg unc. length 13.0 bytes, avg comp. ratio 15.38% over 1 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
   STORE_FORWARD_APP: min: -1 -> -1, max: 0 -> 0, avg unc. length 0.0 bytes, avg comp. ratio 0.00% over 6 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
      RANGE_TEST_APP: min: -1 -> -1, max: 0 -> 0, avg unc. length 0.0 bytes, avg comp. ratio 0.00% over 3 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
      TRACEROUTE_APP: min: 14 -> 11, max: 103 -> 19, avg unc. length 49.2 bytes, avg comp. ratio 50.35% over 28 packets (1 in this interval), 0.1%/0.0% of all packets this interval/ever
    NEIGHBORINFO_APP: min: 14 -> 10, max: 144 -> 96, avg unc. length 32.3 bytes, avg comp. ratio 29.86% over 28 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
         ATAK_PLUGIN: min: 57 -> 44, max: 57 -> 44, avg unc. length 57.0 bytes, avg comp. ratio 22.81% over 1 packets (0 in this interval), 0.0%/0.0% of all packets this interval/ever
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Nathan Clack](https://github.com/nclack/arithcode)'s arithcode repo - would not be possible without his work
- [SoCal Mesh](https://socalmesh.org/) - I'm fortunate to be living in one of the busier and geographically diverse Meshtastic communities
- Meshtastic - it's a fun hobby
