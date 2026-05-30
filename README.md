# Multithreaded Custom Load Balancer

This load balancer is for the mock AI SaaS business website Maltoki. It utilizes the Crow C++ web framework as a frontend layer for taking requests and distributes them across multiple worker server encrypted socket connections.  Each of these clients are given their own thread and queue for processing requests in parallel.
