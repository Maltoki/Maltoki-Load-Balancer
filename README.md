# Multithreaded Custom Load Balancer

This load balancer is for the mock AI SaaS business website Maltoki. I built this load balancer to distribute AI workloads across a raspberry pi cluster. It utilizes the Crow C++ web framework as a frontend layer for taking requests and distributes them across multiple worker server end to end encrypted socket connections.  Each of these clients are given their own thread and queue for processing requests in parallel.  The load balancer and worker servers are both given a symmetric private key to AES encrypt distributed requests using openssl.
