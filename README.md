Code mode availabe under the (Apache License 2.0) from Network Geographics.

This is part of the Infosecter product. This code was used to create a multi-dimensional "flow space" for IP network data.
The flow space dimensions were IP packet properties. The flow space acted as a container such that orthogonal regions of the
space could contain an payload defined by a template parameter. This provide fast queries for orthogonal regions with iteration
over all intersecting regions. That is, if the flow space was created by assigning properties to regions (such as whether packets
are permitted from the IP address and port of one pair of dimensions to the IP address and port of another pair of dimensions),
then a useful query might be asking for those packet handling properties for packets from one subnet to another. The result
would be an iterator that iterated over the intersecting regions (those in both the query region and a defined region) allowing
examination of the properties of the intersecting regions.
