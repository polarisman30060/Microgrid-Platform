# Documentation Standards

## Purpose

This document defines the required structure for project documentation.

The goal is to ensure all documentation is:

- Consistent
- Easy to review
- Easy to maintain
- Easy to expand
- Easy to understand years later

---

# Node Definition Template

Every Node Definition shall contain the following sections.

## Purpose

What is this node?

Why does it exist?

---

## Responsibilities

What functions are owned by this node?

---

## Not Responsible For

What functions are explicitly owned by other nodes or the Coordinator?

---

## Interfaces

What buses, systems, or devices does this node communicate with?

---

## Inputs

What information or energy enters this node?

---

## Outputs

What information or energy leaves this node?

---

## Operating States

What states can this node enter?

Examples:

- OFF
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

---

## Safety Responsibilities

What protective functions are owned by this node?

---

## Coordinator Interaction

What information is exchanged with the Coordinator?

---

## Fault Conditions

What fault conditions may be reported?

---

## Open Questions

Items not yet finalized.

---

# Architecture Documents

Architecture documents should contain:

## Purpose

## Design Philosophy

## Assumptions

## Constraints

## Requirements

## Open Questions

---

# General Rules

Document:

- Responsibilities
- Ownership
- Interfaces
- Assumptions

Do not document implementation details unless they directly affect system architecture.

Prefer defining behavior over defining hardware.

Prefer defining responsibilities over defining algorithms.

The objective is to define what a subsystem must accomplish rather than how it is implemented.