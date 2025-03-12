# Meshtastic Compression Test

[License

This is a *very* rough utility which attempts to gather real-world statistics on the compressibility of Meshtastic traffic using an arithmetic coder I found online and stripped down/cleaned up for embedded use.

The arithmetic coder is written by [Nathan Clack](https://github.com/nclack/arithcode); I do not propose to understand how it works, but he has documented his [source of inspiration](http://www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf) and documented his code in a way that I am sure mathematicians understand, but I can not even begin to comprehend. Still, I was able to distill it down to a small pair of .c and .h files, and have successfully run it on embedded (STM32) platforms. There is still more work to be done to help document and test.

This utility will connect to an MQTT server and subscribe to a single topic. For each Meshtastic MQTT message received, it will extract the raw packet data and then attempt to decrypt it using the default key (`AQ==` which is a shorthand for the actual AES256-CTR key used my Meshtastic, which is `d4f1bb3a20290759f0bcffabcf4e6901`). If decryption is successful it will then compress and then decompress the packet and record some statistics. (**TODO:** If configured to do so, it will publish the statistics to another MQTT topic on the same server.) It will also periodically (every 1000 packets), print out the statistics it's gathered to stdout. The utility does attempt to de-duplicate the MQTT traffic since my use case involves capturing traffic from as wide a net as possible, and a single packet being uplinked several times definitely happens. I didn't want them to skew the compression statistics.

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
            NODEINFO_APP: 31.43% (233 symbols: 35 -> 35 bytes) best: 35 -> 24, worst: 73 -> 56, avg 45.7 bytes, avg ratio 31.01% over 3 packets
            POSITION_APP: 28.57% (243 symbols: 28 -> 28 bytes) best: 28 -> 20, worst: 33 -> 22, avg 32.5 bytes, avg ratio 32.86% over 2 packets
           TELEMETRY_APP: 25.00% (247 symbols: 28 -> 28 bytes) best: 28 -> 21, worst: 28 -> 21, avg 28.0 bytes, avg ratio 25.00% over 1 packets
```

Each line provides some statistics about the message itself, and all messages of that type which have been received to date:

* `NODEINFO_APP` - the type of message

* `31.43% (233 symbols: 35 -> 35 bytes)` - the compression ratio with CDR symbols and byte counts

* `best: 35 -> 24, worst: 73 -> 56` - best and worst compression achieved for this message type

* `avg 45.7 bytes, avg ratio 31.01%` - running averages of the length and compression ratio for this type

* `over 3 packets` - how many packets of this type were analyzed so far

  

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
