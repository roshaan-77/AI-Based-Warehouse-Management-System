# AI-Based Warehouse Management System

A C-based Operating Systems project that demonstrates an adaptive warehouse management simulation using the classic producer-consumer problem. The system combines POSIX threads, mutexes, semaphores, shared memory, persistent users, logging, and a lightweight AI scheduler to show how supplier and retailer activity can be balanced in real time.

## Project Overview

The project models a warehouse where suppliers produce items and retailers consume items from a shared bounded buffer. The core OS concept is producer-consumer synchronization. The system prevents race conditions using mutex locks and controls empty/full buffer access using POSIX semaphores.

An AI scheduler observes the warehouse state and adjusts supplier and retailer delays. This simulates adaptive priority boosting: when the buffer is low, suppliers are boosted; when the buffer is high, retailers are boosted; when the system is balanced, both sides run evenly.

## Key Features

- Producer-consumer warehouse simulation
- Supplier threads acting as producers
- Retailer threads acting as consumers
- Shared warehouse buffer with fixed capacity
- POSIX thread-based concurrency
- Mutex protection for critical sections
- Full and empty semaphores for synchronization
- Shared memory using `shm_open` and `mmap`
- AI-based adaptive scheduling using Q-learning concepts
- Real-time interface for admin, supplier, and retailer workflow
- Persistent admin and user records
- Activity logging with buffer and AI state details
- Operation control for clean demonstrations

## How The AI Helps

The AI is not only displaying warnings. It actively observes the warehouse condition and changes how fast suppliers and retailers operate.

- If the buffer is almost empty, retailers may starve. The AI boosts suppliers by reducing supplier delay.
- If the buffer is almost full, suppliers may block. The AI boosts retailers by reducing retailer delay.
- If both sides are balanced, the AI keeps equal delay for suppliers and retailers.
- Rewards update the Q-table so the system can show which action is becoming more useful for each state.

This means the AI helps reduce starvation, prevents one side from waiting too long, and keeps the warehouse buffer closer to a stable working level.

## Main Components

| File | Purpose |
| --- | --- |
| `main.c` | Starts and controls the warehouse system |
| `warehouse.c`, `warehouse.h` | Shared warehouse buffer, synchronization, and state management |
| `supplier.c`, `supplier.h` | Supplier/producer behavior |
| `retailer.c`, `retailer.h` | Retailer/consumer behavior |
| `ai.c`, `ai.h` | AI scheduler, state detection, rewards, and Q-table logic |
| `ui.c`, `ui.h` | Interactive terminal interface |
| `admin.c`, `admin.h` | Admin account and management functions |
| `user.c`, `user.h` | Supplier and retailer user handling |
| `login.c`, `login.h` | Login and authentication logic |
| `dashboard.c` | Dashboard-focused real-time view |
| `web_ui.c` | Experimental web UI source |
| `Makefile` | Build configuration |

## Requirements

This project is designed for Ubuntu or WSL Ubuntu.

Install the required packages:

```bash
sudo apt update
sudo apt install build-essential libncurses-dev
```

## Build And Run

Open the project folder in Ubuntu:

```bash
cd ~/OS_Project
```

Clean old compiled files:

```bash
make clean
```

Build the project:

```bash
make
```

Run the main warehouse interface:

```bash
./warehouse_system
```

If you only want the dashboard view and it is available after build:

```bash
./dashboard
```

## Default Admin Accounts

The project includes file-based admin credentials. Depending on the current data files, the demo accounts may include:

| Admin | Password |
| --- | --- |
| Roshaan | 0724 |
| Arti | 0530 |
| Abdullah | 0576 |

If the credential files are missing, run the program once so the system can recreate or initialize the required files according to the project logic.

## Suggested Demo Flow

1. Build the project using `make`.
2. Run `./warehouse_system`.
3. Log in as admin.
4. Add suppliers and retailers through the interface.
5. Start the warehouse simulation.
6. Watch the buffer fill and empty in real time.
7. Observe AI state changes such as low buffer, high buffer, balanced state, and starvation conditions.
8. Show how AI actions change supplier and retailer delays.
9. Stop the simulation cleanly.
10. Review logs and final counters.

## Important OS Concepts Demonstrated

- Process/thread synchronization
- Critical section protection
- Race condition prevention
- Producer-consumer problem
- POSIX threads
- POSIX semaphores
- Shared memory IPC
- Real-time monitoring
- Adaptive scheduling behavior

## Report

The final project report is included in the repository under:

```text
docs/OS_Project_Report.docx
```

The report explains the objective, introduction, background, platform and languages, methodology, results, and conclusion.

## Notes

This project is intended for academic demonstration. The AI scheduler uses lightweight Q-learning style logic and delay-based priority boosting instead of changing real OS thread priorities. This keeps the behavior understandable, portable, and suitable for a C-based Operating Systems project demo.
