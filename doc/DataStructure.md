# Data Structures

HeartyRabbit stores its data in two places: Redis database and local
filesystem.

## Terminology

[Blob](https://en.wikipedia.org/wiki/Binary_large_object): a blob stands for
"**B**inary **L**arge **OB**jects". This is the basic unit in HeartyRabbit.
It basically represents a bunch of bytes. For an image album, it represents an
image.

Collection: it is a container of multiple blobs, like a directory in a file
system. Each blob in a collection has its own access permission. The same blob
can be put in more than one collection, and they can be assigned different
access permissions.

## Blob

Blobs are identified by their hash value, which is also known as _blob ID_.
The hash algorithm used in HeartyRabbit is [Blake2](https://blake2.net/).
Hash length is 20 bytes. Blake2 is chosen because there's known collision
yet, and it can generated variable length hash values.

Blobs are stored in the local filesystem, not in Redis. This is because
Redis stores data in memory, and blobs are generated too large to be fit
in memory. In general, blobs are not read from disk very often, because
HeartyRabbit uses HTTP cache aggressively.

Since blobs can be large, it may be expensive to sending them across the
network frequently. Instead, HeartyRabbit can generate different _renditions_,
which can also logically represent the blob with a lower quality to save
bandwidth. For images, a rendition is often a smaller version of the original
image, and possibly with a lower JPEG quality setting. For audio, it can be
encoded in a different codec (e.g. OPUS) that can achieve better compression
ratio, and possibly with a lower bitrate. HeartyRabbit will use a different
rendition for a blob in different cases. For example, an image gallery page
will use thumbnails, which is a rendition with a small size.

The original blob used to calculate the ID is called the _master rendition_.
It should be the most accurate choice to represent the blob. HeartyRabbit
uses the master rendition to generate others on demand when requested.

Note that although a blob has multiple renditions, it only has one ID, which
is the 20-byte Blake2 hash of the master rendition. The hash of other rendition
is never calculated.

Since a blob is identified by its hash, which is calculated by its content,
a blob cannot be changed once it's created. The same ID always refer to the
same blob. This is a property of a cryptographically safe hash. This also
means to "update" a blob, we can only create a new one and replace all
references of the old one with the new one.
