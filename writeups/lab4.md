Lab 4 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

I worked with or talked about this assignment with: [please list other sunetids]

Program Structure and Design of the TCPConnection:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

优化方法：把ByteStream的std::string改成BufferList
感觉优化之后反而还更慢了

三次跑分
优化前

CPU-limited throughput                : 4.21 Gbit/s
CPU-limited throughput with reordering: 2.87 Gbit/s

CPU-limited throughput                : 4.18 Gbit/s
CPU-limited throughput with reordering: 2.76 Gbit/s

CPU-limited throughput                : 3.71 Gbit/s
CPU-limited throughput with reordering: 2.42 Gbit/s

优化后

CPU-limited throughput                : 3.20 Gbit/s
CPU-limited throughput with reordering: 3.01 Gbit/s

CPU-limited throughput                : 3.36 Gbit/s
CPU-limited throughput with reordering: 2.38 Gbit/s

CPU-limited throughput                : 3.30 Gbit/s
CPU-limited throughput with reordering: 2.88 Gbit/s