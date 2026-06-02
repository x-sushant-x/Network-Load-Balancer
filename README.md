### Layer 4 (TCP) Load Balancer

A performance oriented event-driven Layer 4 TCP load balancer written from scratch in pure C using non-blocking sockets and kernel event polling. This proejct does not spawn a new thread for each request. Instead it implements an Event Driven I/O (backbone of popular proxies like HA & Nginx) from scratch that allow us to handle several requests in a single thread. It scales far better than the traditional thread-per-request model and use lesser CPU resources.

> [!WARNING]
> Event polling is implemented using **kqueue** syscall provided by macOS kernel. It will not work on Linux as Linux kernel don't have **kqueue** instead it provide same functionality with **epoll** syscall.


### How it works?
The entire server runs on a single event loop powered by kqueue.

* No thread-per-connection model.
* No blocking I/O.

Instead:
* The kernel notifies when sockets become readable/writable.
* The server reacts asynchronously.
* Thousands of connections can be multiplexed efficiently.

> [!NOTE]
> This project is intentionally written in C for 2 reasons.
> 1. Understand things from absolute ground level.
> 2. Languages like Go already have Event Polling implemented internally. Learning may get abstracted.


> [!CAUTION]
> This is just a learning project.
