Bigfoot
=======

About
-----

Bigfoot is a compression algorithm inspired by the Gorilla
compression algorithm, but optimized for integer-based data
such as terrain height maps. These datasets typically
contain arrays of height values that change gradually,
resembling smooth gradients like rolling hills.

Compression is achieved using a combination of delta
encoding, variable-bit encoding (including a sign bit), and
a simple control scheme. The control scheme improves
efficiency when encoding smooth gradients by allowing the
delta size to adjust gradually as new values are processed.
When an abrupt change occurs, such as a cliff, the scheme
can reset the delta size to adapt to the sudden variation.

Control Scheme
--------------

The control scheme defines the delta size used in the
variable-bit encoding as the number of bits required to
represent the difference between adjacent values. Because
each delta includes a sign bit, its size must be at least
two bits. A delta value of zero is treated as a special case
to optimize repeated values and is encoded with a delta size
of zero.

The control codes are defined.

	00: Same delta size
	01: Increment delta size
	10: Decrement delta size
	11xxxx:  Reset delta size (short)
	11xxxxx: Reset delta size (int)

Where the binary reset size field is converted to a delta
size.

	delta_size = xxxx  + 1; // short
	delta_size = xxxxx + 1; // int

A heuristic is applied to improve the compression ratio by
minimizing reset operations, which introduce additional
overhead. Reset operations are costly since they require an
extra 4-bit (short) or 5-bit (int) size field.

The control scheme supports two special cases for
dynamically adjusting the delta size:

**Increasing the delta size:** When the delta size must
increase to represent larger deltas, the control code "01"
increments the delta size by 2, effectively quadrupling the
representable delta range. This heuristic introduces a
one-bit overhead when a smaller increment would have
sufficed, but can save up to four bits when a two-bit
increase would otherwise trigger a reset. The temporary
one-bit overhead is absorbed as subsequent values are
encoded. For larger required increases (more than two bits),
the reset operation is used instead. The special delta size
0 is treated as a separate case and is incremented by 4, both
to introduce the sign bit and to achieve the same fourfold
expansion of the representable delta range.

**Decreasing the delta size:** When smaller deltas are
encountered, the control code "10" decrements the delta size
by 1, halving the representable range. Stepwise reduction
may introduce minor overhead when a larger decrease would
have been more efficient in the short term. However, by
slowly converging, the probability of triggering a reset
operation due to overshooting the required delta size is
reduced. When the required reduction is greater than or
equal to four bits, the reset operation is applied instead.
For example, a three-bit reduction incurs two bits of
overhead for the current value and one bit for the next,
with zero overhead thereafter, yielding a net savings of one
bit compared to performing a reset. In contrast, a four-bit
reduction would incur 3 bits of overhead for the current
value, 2 bits for the second value, 1 bit for the third
value and zero overhead thereafter. This yields a net loss
of 2 bits compared to performing a reset. For sequences of
identical values, a special case allows delta size 2 to be
reduced to 0, ensuring optimal encoding of repeated values.

Bigfoot does not include a dedicated control code to
optimize long runs of identical values unlike the
Gorilla compression algorithm. Because Bigfoot continuously
adapts the delta size and typically operates on smoothly
varying height values, the encoding naturally converges to
emitting the control code "00" without any additional
delta bits for repeated values. As a result, adding such a
special case would provide little extra benefit in Bigfoot
and would exhaust the remaining control-code space.

The output buffer is initialized by storing the first input
value directly, and all subsequent values are compressed
using the control scheme. The delta size is initialized to
zero so that small deltas can be encoded immediately without
requiring a reset code.

References
----------

* [The Simple Beauty of XOR Floating Point Compression](https://clemenswinter.com/2024/04/07/the-simple-beauty-of-xor-floating-point-compression/)

License
=======

The Bigfoot library was developed by
[Jeff Boody](mailto:jeffboody@gmail.com)
under The MIT License.

	Copyright (c) 2025 Jeff Boody

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
