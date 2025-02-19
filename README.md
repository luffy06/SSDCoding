# Coding Scheme

## State
* Single State: For each three fixed-length (18 KB) page, we combine the bits vertically (LSB, CSB, and MSB) to form the single state, where each single state ranges from 0 to 7 in octal.
* Grouped State: For a sequence of single states, we group a fixed number of single states to form the grouped state. For example, if we group two states, we can get the double state, where each double state ranges from 00 to 77 in octal.

## Goal
Map the single states to the specific single states as much as possible, in order to reduce the bit errors.

## Algorithm
General workflow:
1. Count the frequency of each grouped state.
2. Sort the grouped states based on the frequency.
3. Map the grouped states using different strategies.
