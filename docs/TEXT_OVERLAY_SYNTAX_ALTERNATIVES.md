# Text Overlay Syntax Alternatives Analysis

## Goal
Consolidate `-S`, `-G`, `-O` text input and styling options (`-F`, `-A`, `-I`, `-C`, `-B`, `-M`, `-X`, `-D`, `-V`) into a single unified `-T` parameter with embedded styling information.

## Current Complexity
Current approach requires separate flags for text and styling:
```bash
bin/pisstvpp2 -i photo.jpg \
  -S "W5ABC" -F 20 -A center -I top -C white -B blue -M opaque -X 8 -V center \
  -G "EM12ab" -F 16 -A center -I bottom -C yellow -B black -M semi -X 4
```

## Proposed Alternatives

### Option 1: Pipe-Separated Key-Value (RECOMMENDED - Most Shell-Friendly)
**Syntax:**
```bash
-T "text|font-size|alignment|position|text-color|bg-color|bg-mode|padding|border|v-align"
-T "W5ABC|20|center|top|white|blue|opaque|8|2|center"
```

**Verbose form:**
```bash
-T "text=W5ABC|size=20|align=center|pos=top|color=white|bg=blue|mode=opaque|pad=8|border=2"
```

**Pros:**
- ✅ Natural for CLI (pipe character well-known)
- ✅ Easy to parse (split by `|`)
- ✅ Very compact
- ✅ Order-independent with verbose form
- ✅ Named parameters prevent field order confusion

**Cons:**
- ⚠️ Positional version requires remembering order
- ⚠️ Positional version less human-readable

**Example:**
```bash
bin/pisstvpp2 -i photo.jpg \
  -T "W5ABC|size=20|align=center|pos=top|color=white|bg=blue" \
  -T "EM12ab|size=16|align=center|pos=bottom|color=yellow|bg=black"
```

---

### Option 2: Space-Separated Key=Value (Most POSIX-like)
**Syntax:**
```bash
-T "text=W5ABC size=20 align=center pos=top color=white bg=blue mode=opaque pad=8"
```

**Pros:**
- ✅ POSIX-standard format (like environment variables)
- ✅ Very readable
- ✅ Familiar to *nix users
- ✅ Order-independent

**Cons:**
- ⚠️ Requires careful shell quoting with spaces
- ⚠️ Easy to break if text contains spaces (but text is first value)

**Example:**
```bash
bin/pisstvpp2 -i photo.jpg \
  -T "text='W5ABC' size=20 align=center pos=top color=white bg=blue" \
  -T "text='EM12ab' size=16 align=center pos=bottom color=yellow bg=black"
```

---

### Option 3: CSS-Inspired Curly Brace Syntax
**Syntax:**
```bash
-T "W5ABC{size:20}{align:center}{pos:top}{color:white}{bg:blue}{mode:opaque}"
```

**Pros:**
- ✅ Compact and distinctive
- ✅ Familiar to web developers
- ✅ Clear separation of text from styling
- ✅ No quoting issues for spaces in property names

**Cons:**
- ⚠️ Less familiar to system admins
- ⚠️ Braces may need escaping in some shells
- ⚠️ More complex parsing (regex or bracket matching)

**Example:**
```bash
bin/pisstvpp2 -i photo.jpg \
  -T "W5ABC{size:20}{align:center}{pos:top}{color:white}{bg:blue}{mode:opaque}{pad:8}" \
  -T "EM12ab{size:16}{align:center}{pos:bottom}{color:yellow}{bg:black}{mode:semi}{pad:4}"
```

---

### Option 4: Colon-Separated Compact (Most Minimal)
**Syntax:**
```bash
-T "W5ABC:20:center:top:white:blue:opaque:8:2:center"
```

**Pros:**
- ✅ Most compact form
- ✅ Minimal typing
- ✅ Easy shell handling

**Cons:**
- ❌ Hard to remember field order
- ❌ Error-prone (wrong order breaks styling)
- ❌ No field names = cryptic
- ❌ Difficult to omit optional fields

**Not Recommended** - Too error-prone for CLI

---

### Option 5: INI-Style Within Curly Braces
**Syntax:**
```bash
-T "{text:W5ABC, size:20, align:center, pos:top, color:white, bg:blue, mode:opaque, pad:8}"
```

**Pros:**
- ✅ Very readable
- ✅ Known structure
- ✅ All styling grouped together

**Cons:**
- ⚠️ Verbose
- ⚠️ Requires bash escaping of braces/commas

**Example:**
```bash
bin/pisstvpp2 -i photo.jpg \
  -T "{text:W5ABC, size:20, align:center, pos:top, color:white, bg:blue}" \
  -T "{text:EM12ab, size:16, align:center, pos:bottom, color:yellow, bg:black}"
```

---

### Option 6: JSON-Inspired (Clean but Verbose)
**Syntax:**
```bash
-T '{"text":"W5ABC","size":20,"align":"center","pos":"top","color":"white","bg":"blue"}'
```

**Pros:**
- ✅ Standard, parseable format
- ✅ Very clear and structured
- ✅ Can validate against schema
- ✅ Language-independent

**Cons:**
- ⚠️ Very verbose
- ⚠️ Lots of quoting needed
- ⚠️ Easy shell quoting mistakes
- ⚠️ Overkill for CLI parameter

**Example:**
```bash
bin/pisstvpp2 -i photo.jpg \
  -T '{"text":"W5ABC","size":20,"align":"center","pos":"top","color":"white","bg":"blue"}' \
  -T '{"text":"EM12ab","size":16,"align":"center","pos":"bottom","color":"yellow","bg":"black"}'
```

---

### Option 7: YAML-Inspired Hyphen-Separated
**Syntax:**
```bash
-T "W5ABC - size: 20 - align: center - pos: top - color: white - bg: blue"
```

**Pros:**
- ✅ Markdown-familiar syntax
- ✅ Very readable

**Cons:**
- ⚠️ Uncommon for CLI use
- ⚠️ Hyphen-double-space pattern is non-standard
- ⚠️ Extra parsing complexity

---

### Option 8: Question-Mark Prefix for Properties (Unique Hybrid)
**Syntax:**
```bash
-T "W5ABC?size=20?align=center?pos=top?color=white?bg=blue"
```

**Pros:**
- ✅ Compact
- ✅ Text first, clear separation
- ✅ No shell escaping issues

**Cons:**
- ⚠️ Non-standard convention
- ⚠️ `?` may have shell meaning in some contexts

---

## Detailed Comparison Table

| Option | Syntax | Readability | Parsability | Shell-Safe | Recommended |
|--------|--------|------------|------------|-----------|------------|
| **1. Pipe + Key=Value** | `text\|size=20\|align=center` | ⭐⭐⭐⭐ Good | ⭐⭐⭐⭐⭐ Easy | ✅ Yes | ✅ **YES** |
| 2. Space + Key=Value | `text=W5ABC size=20` | ⭐⭐⭐⭐⭐ Excellent | ⭐⭐⭐⭐ Good | ⚠️ Needs quotes | ✅ YES (Alt) |
| 3. CSS Braces | `W5ABC{size:20}` | ⭐⭐⭐ Fair | ⭐⭐⭐ Medium | ⚠️ Escape braces | ⚠️ Maybe |
| 4. Colon Compact | `W5ABC:20:center` | ⭐⭐ Poor | ⭐⭐⭐ Medium | ✅ Yes | ❌ NO |
| 5. INI Braces | `{text:W5ABC, size:20}` | ⭐⭐⭐⭐ Good | ⭐⭐⭐ Medium | ⚠️ Complex | ⚠️ Maybe |
| 6. JSON | `{"text":"W5ABC"}` | ⭐⭐⭐⭐ Good | ⭐⭐⭐⭐⭐ Easy | ❌ Quoting Hell | ❌ NO |
| 7. YAML Dash | `W5ABC - size: 20` | ⭐⭐⭐⭐ Good | ⭐⭐ Hard | ✅ Yes | ❌ NO |
| 8. Question-Mark | `W5ABC?size=20` | ⭐⭐⭐ Fair | ⭐⭐⭐⭐ Good | ⚠️ Context-dependent | ⚠️ Maybe |

---

## Final Recommendations

### 🥇 **BEST OPTION: #1 - Pipe-Separated with Named Properties**

**Why:**
- Balances readability with CLI simplicity
- Named properties prevent field-order errors
- Pipe character has established meaning in CLI contexts
- Trivial to parse (simple string split)
- Scales well with future properties

**Implementation:**
```bash
# Simple example
-T "W5ABC|size=20|align=center|pos=top|color=white"

# Complex example with all properties
-T "W5ABC|size=20|align=center|pos=top|color=white|bg=blue|mode=opaque|pad=8|border=2|v-align=center"

# Multiple overlays
bin/pisstvpp2 -i photo.jpg \
  -T "W5ABC|size=20|align=center|pos=top|color=white|bg=blue" \
  -T "EM12ab|size=16|align=center|pos=bottom|color=yellow|bg=black"
```

---

### 🥈 **RUNNER-UP: #2 - Space-Separated with Named Properties**

**Why:**
- Very POSIX-standard
- Most familiar to Linux sysadmins
- Excellent readability
- Easy to remember format

**Implementation:**
```bash
-T "text=W5ABC size=20 align=center pos=top color=white bg=blue mode=opaque"
```

**Note:** Requires careful quoting with spaces in text:
```bash
-T "text='My Station ID' size=20 align=center"  # For text with spaces
```

---

## Property Shorthand Reference

To make both syntax options more concise, we could support abbreviated property names:

| Full Name | Shorthand | Example |
|-----------|-----------|---------|
| text | t | `t=W5ABC` |
| size | s | `s=20` |
| align | a | `a=center` |
| position | p | `p=top` |
| color | c | `c=white` |
| background | bg | `bg=blue` |
| mode | m | `m=opaque` |
| padding | pd | `pd=8` |
| border | b | `b=2` |
| v-align | va | `va=center` |

**Shorthand Example:**
```bash
-T "W5ABC|s=20|a=center|p=top|c=white|bg=blue"
```

---

## Proposed Implementation Strategy

1. **Add new `-T` flag** alongside existing `-S`, `-G`, `-O` for backward compatibility
2. **Parse `-T` parameter** using selected syntax
3. **Default values** for omitted properties (size=12, align=center, pos=center, color=white, etc.)
4. **Deprecation path** - Keep old flags working, gently encourage `-T` in documentation
5. **Phase 2** - Eventually retire `-S`, `-G`, `-O` flags in future version

---

## Example Configurations

### Option 1 Style (Recommended)
```bash
# Simple callsign overlay
-T "W5ABC|s=20|p=top|c=white|bg=blue"

# Full styling
-T "W5ABC|s=20|a=center|p=top|c=white|bg=blue|m=opaque|pd=8|b=2"

# Complete command
bin/pisstvpp2 -i photo.jpg \
  -T "W5ABC|s=20|p=top|c=white|bg=blue" \
  -T "EM12ab|s=16|p=bottom|c=yellow|bg=black" \
  -o output.wav
```

### Option 2 Style (Alternative)
```bash
# Simple overlay
-T "text=W5ABC size=20 pos=top color=white bg=blue"

# With abbreviations
-T "t=W5ABC s=20 p=top c=white bg=blue"

# Complete command
bin/pisstvpp2 -i photo.jpg \
  -T "t=W5ABC s=20 p=top c=white bg=blue" \
  -T "t=EM12ab s=16 p=bottom c=yellow bg=black" \
  -o output.wav
```

---

## Conclusion

**Recommendation:** Use **Option 1 (Pipe-Separated with Named Properties)** for the best balance of:
- ✅ Shell-friendliness
- ✅ Readability
- ✅ Ease of parsing
- ✅ Scalability
- ✅ Low error rate

The pipe character naturally suggests "separate items" in CLI culture, and the key=value format prevents field-order confusion.

