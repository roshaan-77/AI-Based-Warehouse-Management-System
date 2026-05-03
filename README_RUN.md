# Adaptive Warehouse Management System

## Ubuntu setup

```bash
sudo apt update
sudo apt install build-essential libncurses-dev
make clean
make
```

## Recommended final demo flow

Start the professional Ubuntu terminal interface:

```bash
./warehouse_system
```

Inside the interface:

- Login as admin.
- Create supplier and retailer accounts.
- Use Up/Down to select saved users.
- Start one selected user or start all suppliers/retailers.
- Set the operation limit.
- Stop or reset the simulation.
- Use Tab to switch between Overview, AI Learning, and Logs.

The interface shows the live dashboard: buffer state, producer-consumer counts, waiting/blocking threads, AI state, AI action, rewards, delays, Q-table learning, and recent system logs.

Main keys:

```text
A      Admin login
S      Add supplier
R      Add retailer
Up/Down Select saved user
Enter  Start selected user
P      Start all suppliers
O      Start all retailers
L      Set operation limit
X      Stop simulation
C      Reset runtime
Tab    Switch dashboard view
Q      Quit
```

## Optional observer dashboard

If you want a second terminal only for display, keep `./warehouse_system` running and run this in another terminal:

```bash
./dashboard
```

Dashboard keys:

- `S` stops the simulation
- `Q` closes the dashboard

## What the dashboard proves

- The buffer display shows the producer-consumer problem in real time.
- Supplier and retailer waiting counters show blocking/starvation pressure.
- Produced/consumed counters show throughput.
- AI state shows whether the buffer is low, balanced, or high.
- AI action shows whether it boosts suppliers, boosts retailers, or balances both.
- Reward and Q-table values show that the AI is learning from decisions.
- Producer and retailer delays show how AI simulates priority boosting.

## Old text menu backup

The older terminal menu is still available for backup:

```bash
./warehouse_system --legacy
```

## Quick automatic test mode

This mode is only for fast testing, not the main final presentation:

```bash
./warehouse_system --demo 2 2 60
```
