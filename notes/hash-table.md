# Hash Table

Well, the moment has come. I never made a Hash Map algorithm, but in this scenario I have to save both fd and sockaddr (
I could make a simply linked list, but it's a way to learn new things).

Let's start with a little bit of theory.

A *Hash Table* is a data structure also called **dictionary** or **map**. It maps *keys* to *values* thanks to a **hash
function** that computes and *index* (**hash code**) into an array of **buckets**.

One problem could be the *hash collision* where the hash function computes the same index for different values. A fix
could be the **chaining** method, where for the same hash code you can make a linked list and append the index. Then the
lookup function will go through the list and find the key.

In a Hash Map you can insert, delete and lookup (simply search).

### Hash function

The easiest way... don't judge me.

- Integer keys:
  $$ hash(\text{key}) = \text{key} \mod \text{table\_dim} $$

- String keys:
  $$ hash(key) = \sum_{i=0}^{len(key) - 1} ascii\_value(key[i]) * prime\_number$$
