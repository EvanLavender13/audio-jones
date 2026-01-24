---
description: Iterative idea shaping through short questions - narrows vague concepts into a single clear direction
argument-hint: Starting idea (optional, can be blank)
---

# Brainstorm

Shape a vague idea into a single clear concept through short back-and-forth. Claude asks, user steers. No walls of text. No lists of suggestions.

## Rules

- **One question per message.** Wait for the answer.
- **Max 3 sentences per response.** Acknowledge, then ask.
- **Never propose effects/features unprompted.** Ask what the user finds interesting.
- **Never list options.** Narrow by asking about qualities, not presenting menus.
- **End with one concept.** Not two. Not "A or B." One.

---

## Phase 1: Orient

Read the effects inventory: @docs/effects.md

If $ARGUMENTS is blank:
- Ask what visual quality or mood they want to explore (geometric, organic, chaotic, textured, etc.)

If $ARGUMENTS has a vague idea:
- Restate it in one sentence. Ask what aspect matters most.

---

## Phase 2: Narrow

Ask questions that eliminate directions. Examples:

- "Does this react to audio, or run independently?"
- "Organic and flowing, or rigid and geometric?"
- "Applied to the existing image, or generates its own geometry?"
- "Subtle texture, or dramatic transformation?"
- "Closest existing effect in the inventory?"

One question at a time. Each answer eliminates half the space. Stop when the concept is specific enough to research.

---

## Phase 3: Confirm

State the concept in 1-2 sentences. Ask: "Research this?"

If yes → tell user: `/research <concept-name>`

If no → return to Phase 2 with one clarifying question.

---

## Constraints

- Do NOT search the web or fetch URLs
- Do NOT write files
- Do NOT propose implementations
- Do NOT mention pipeline stages, config structs, or code
- Output is a concept direction, not a technical spec
