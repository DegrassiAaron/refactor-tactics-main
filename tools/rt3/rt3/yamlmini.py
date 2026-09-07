"""Parser YAML del solo sottoinsieme che una roadmap RT3 usa. Libreria standard.

Perche' non PyYAML: `tools/rt3/README.md` dichiara «solo libreria standard», e
`AGENTS.md` §9 vieta di introdurre un package manager senza una decisione esplicita.
PyYAML risulta installato su QUESTA macchina, ma i tre workspace sono tre checkout che
possono finire su macchine diverse, e una roadmap che si apre su una workstation e non
sull'altra e' il difetto peggiore di tutti: si manifesta lontano da qui.

⚠️ Questo NON e' un parser YAML. E' il parser di un sottoinsieme, e rifiuta tutto il
resto invece di indovinarlo:

    mapping per indentazione        chiave: valore
    liste                           - elemento
    mapping dentro lista            - chiave: valore
    scalari                         stringa, 12, 1.5, true, false, null, ~
    stringhe quotate                'a: b'   "a: b"
    flow inline                     [1, 2]   {a: 1, b: 2}
    commenti                        # fino a fine riga, e in coda a un valore

    NON supportati, e ognuno alza YamlError con numero di riga:
    anchor (&) alias (*) tag (!) merge (<<) documenti multipli (---)
    block scalar (| >) chiavi complesse (? ) chiavi duplicate

La regola e' fail-closed: se il documento usa qualcosa che questo parser non capisce,
l'errore lo dice e nomina la riga. Un parser che ignorasse in silenzio una `&anchor`
produrrebbe una roadmap valida e SBAGLIATA - dipendenze mancanti che nessun gate vede.

⛔ Il cross-check contro PyYAML esiste, ma vive nei test (`test_yamlmini.py`), non qui:
quando PyYAML c'e', i test confrontano i due risultati sullo stesso documento. E' cosi'
che questo modulo resta onesto senza dipendere da quello.
"""

import re

__all__ = ["YamlError", "parse", "parse_file"]


class YamlError(ValueError):
    """Documento fuori dal sottoinsieme supportato, o malformato.

    Porta sempre il numero di riga (1-based): senza, un errore su una roadmap di 200
    righe costringe a bisecare a mano.
    """

    def __init__(self, message, line=None):
        self.line = line
        if line is not None:
            message = "riga {}: {}".format(line, message)
        super().__init__(message)


# ---------------------------------------------------------------------------
# Scalari
# ---------------------------------------------------------------------------

_INT_RE = re.compile(r"^[+-]?\d+$")
_FLOAT_RE = re.compile(r"^[+-]?(\d+\.\d*|\.\d+|\d+)([eE][+-]?\d+)?$")

#: `y`/`n`/`on`/`off` sono deliberatamente ASSENTI. YAML 1.1 li rende booleani e YAML
#: 1.2 no; una roadmap che scrivesse `homeWork: NO` cambierebbe significato a seconda
#: del parser. Qui restano stringhe, e chi vuole un booleano scrive `true` o `false`.
_TRUE = ("true", "True", "TRUE")
_FALSE = ("false", "False", "FALSE")
_NULL = ("null", "Null", "NULL", "~", "")

_REJECTED_PREFIXES = (
    ("&", "anchor YAML (&) non supportata"),
    ("*", "alias YAML (*) non supportato"),
    ("!", "tag YAML (!) non supportato"),
    ("|", "block scalar (|) non supportato"),
    (">", "block scalar (>) non supportato"),
    ("? ", "chiave complessa (?) non supportata"),
)


def _strip_comment(text):
    """Toglie il commento in coda rispettando le stringhe quotate.

    ⚠️ Un `text.split('#')[0]` romperebbe `note: "vedi #2272"`, che in una roadmap di
    RefactorTactics e' la forma normale di citare una issue.
    """
    quote = None
    for i, ch in enumerate(text):
        if quote:
            if ch == quote:
                quote = None
            continue
        if ch in "'\"":
            quote = ch
            continue
        if ch == "#":
            # Un `#` e' un commento solo se preceduto da spazio o a inizio riga:
            # `colore: #fff` non e' un commento vuoto, e' un valore.
            if i == 0 or text[i - 1] in " \t":
                return text[:i]
    return text


def _scalar(raw, line):
    """Converte uno scalare. Le stringhe quotate restano stringhe, sempre."""
    text = raw.strip()

    if len(text) >= 2 and text[0] == text[-1] and text[0] in "'\"":
        inner = text[1:-1]
        if text[0] == '"':
            # Solo gli escape che servono. Uno `\x41` non compare in una roadmap, e
            # inventarne il supporto sarebbe codice non provato.
            inner = (
                inner.replace('\\"', '"').replace("\\n", "\n").replace("\\\\", "\\")
            )
        else:
            inner = inner.replace("''", "'")
        return inner

    for prefix, why in _REJECTED_PREFIXES:
        if text.startswith(prefix):
            raise YamlError(why + ": {!r}".format(text), line)

    if text in _NULL:
        return None
    if text in _TRUE:
        return True
    if text in _FALSE:
        return False
    if _INT_RE.match(text):
        return int(text)
    if _FLOAT_RE.match(text) and not _INT_RE.match(text):
        return float(text)
    return text


# ---------------------------------------------------------------------------
# Flow inline: [a, b]  {a: 1}
# ---------------------------------------------------------------------------


def _split_flow(body, line):
    """Divide sulle virgole di PRIMO livello, rispettando quote e annidamento."""
    parts, buf, depth, quote = [], [], 0, None
    for ch in body:
        if quote:
            buf.append(ch)
            if ch == quote:
                quote = None
            continue
        if ch in "'\"":
            quote = ch
            buf.append(ch)
            continue
        if ch in "[{":
            depth += 1
        elif ch in "]}":
            depth -= 1
            if depth < 0:
                raise YamlError("parentesi di chiusura senza apertura", line)
        if ch == "," and depth == 0:
            parts.append("".join(buf))
            buf = []
            continue
        buf.append(ch)
    if quote:
        raise YamlError("stringa quotata non chiusa", line)
    if depth != 0:
        raise YamlError("parentesi non bilanciate nel flow", line)
    tail = "".join(buf)
    if tail.strip():
        parts.append(tail)
    return parts


def _parse_flow(text, line):
    text = text.strip()
    if text.startswith("["):
        if not text.endswith("]"):
            raise YamlError("lista inline non chiusa: {!r}".format(text), line)
        body = text[1:-1].strip()
        if not body:
            return []
        return [_parse_value(p, line) for p in _split_flow(body, line)]

    if text.startswith("{"):
        if not text.endswith("}"):
            raise YamlError("mapping inline non chiuso: {!r}".format(text), line)
        body = text[1:-1].strip()
        result = {}
        if not body:
            return result
        for part in _split_flow(body, line):
            if ":" not in part:
                raise YamlError(
                    "voce di mapping inline senza ':': {!r}".format(part.strip()), line
                )
            key, value = _split_key(part, line)
            if key in result:
                raise YamlError("chiave duplicata nel mapping inline: {}".format(key), line)
            result[key] = _parse_value(value, line)
        return result

    raise YamlError("flow non riconosciuto: {!r}".format(text), line)


def _parse_value(text, line):
    text = text.strip()
    if text.startswith("[") or text.startswith("{"):
        return _parse_flow(text, line)
    return _scalar(text, line)


def _split_key(text, line):
    """Separa `chiave: resto` sul primo `:` fuori da quote e da flow."""
    quote, depth = None, 0
    for i, ch in enumerate(text):
        if quote:
            if ch == quote:
                quote = None
            continue
        if ch in "'\"":
            quote = ch
            continue
        if ch in "[{":
            depth += 1
        elif ch in "]}":
            depth -= 1
        elif ch == ":" and depth == 0:
            # `a:b` senza spazio NON e' una coppia in YAML: e' lo scalare "a:b".
            if i + 1 < len(text) and text[i + 1] not in " \t":
                continue
            key = text[:i].strip()
            if len(key) >= 2 and key[0] == key[-1] and key[0] in "'\"":
                key = key[1:-1]
            return key, text[i + 1 :].strip()
    raise YamlError("attesa una coppia 'chiave: valore', trovato {!r}".format(text), line)


# ---------------------------------------------------------------------------
# Righe
# ---------------------------------------------------------------------------


class _Line(object):
    __slots__ = ("indent", "text", "number")

    def __init__(self, indent, text, number):
        self.indent = indent
        self.text = text
        self.number = number

    def __repr__(self):  # pragma: no cover - diagnosi
        return "_Line({}, {!r}, {})".format(self.indent, self.text, self.number)


def _tokenize(source):
    lines = []
    for number, raw in enumerate(source.splitlines(), start=1):
        if "\t" in raw[: len(raw) - len(raw.lstrip("\t "))]:
            raise YamlError(
                "indentazione con TAB: YAML ammette solo spazi, e un tab qui "
                "cambierebbe la struttura in modo invisibile",
                number,
            )
        stripped = _strip_comment(raw).rstrip()
        if not stripped.strip():
            continue
        content = stripped.lstrip(" ")
        indent = len(stripped) - len(content)
        if content.startswith("---") or content.startswith("..."):
            raise YamlError(
                "marcatore di documento ({}): questo parser legge un documento "
                "solo".format(content.strip()),
                number,
            )
        if content.startswith("<<"):
            raise YamlError("merge key (<<) non supportata", number)
        lines.append(_Line(indent, content, number))
    return lines


# ---------------------------------------------------------------------------
# Blocchi
# ---------------------------------------------------------------------------


def _parse_block(lines, start, indent):
    """Legge il blocco a `indent` a partire da `start`. Ritorna (valore, indice)."""
    if start >= len(lines):
        return None, start
    if lines[start].text.startswith("- "):
        return _parse_sequence(lines, start, indent)
    if lines[start].text == "-":
        return _parse_sequence(lines, start, indent)
    return _parse_mapping(lines, start, indent)


def _parse_sequence(lines, start, indent):
    items = []
    i = start
    while i < len(lines):
        line = lines[i]
        if line.indent < indent:
            break
        if line.indent > indent:
            raise YamlError(
                "indentazione inattesa dentro una lista (attesi {} spazi, trovati "
                "{})".format(indent, line.indent),
                line.number,
            )
        if not (line.text == "-" or line.text.startswith("- ")):
            break

        body = line.text[1:].strip()
        child_indent = _next_indent(lines, i, indent)

        if not body:
            # `-` nudo: il valore sta nelle righe indentate sotto.
            if child_indent is None:
                items.append(None)
                i += 1
                continue
            value, i = _parse_block(lines, i + 1, child_indent)
            items.append(value)
            continue

        # `- chiave: valore` apre un mapping il cui indent e' quello di `chiave`.
        if _looks_like_pair(body):
            item_indent = line.indent + (len(line.text) - len(line.text[1:].lstrip()))
            value, i = _parse_mapping_from_inline(lines, i, item_indent, body)
            items.append(value)
            continue

        if child_indent is not None:
            raise YamlError(
                "elemento di lista con valore scalare {!r} seguito da righe "
                "indentate: ambiguo".format(body),
                line.number,
            )
        items.append(_parse_value(body, line.number))
        i += 1
    return items, i


def _looks_like_pair(body):
    if body.startswith("[") or body.startswith("{"):
        return False
    try:
        _split_key(body, 0)
    except YamlError:
        return False
    return True


def _parse_mapping_from_inline(lines, index, indent, first_body):
    """Mapping che comincia sulla riga della lista: `- id: A1`."""
    line = lines[index]
    key, rest = _split_key(first_body, line.number)
    result = {}
    i = index + 1

    if rest:
        result[key] = _parse_value(rest, line.number)
    else:
        child_indent = _next_indent(lines, index, indent)
        if child_indent is None:
            result[key] = None
        else:
            result[key], i = _parse_block(lines, index + 1, child_indent)

    rest_map, i = _parse_mapping(lines, i, indent, existing=result)
    return rest_map, i


def _next_indent(lines, index, indent):
    """Indentazione del blocco figlio della riga `index`, o None se non c'e'."""
    if index + 1 >= len(lines):
        return None
    nxt = lines[index + 1]
    if nxt.indent > indent:
        return nxt.indent
    return None


def _parse_mapping(lines, start, indent, existing=None):
    result = existing if existing is not None else {}
    i = start
    while i < len(lines):
        line = lines[i]
        if line.indent < indent:
            break
        if line.indent > indent:
            raise YamlError(
                "indentazione inattesa (attesi {} spazi, trovati {})".format(
                    indent, line.indent
                ),
                line.number,
            )
        if line.text == "-" or line.text.startswith("- "):
            break

        key, rest = _split_key(line.text, line.number)
        if key in result:
            raise YamlError(
                "chiave duplicata: {!r}. Una roadmap con due voci omonime "
                "perderebbe la prima in silenzio.".format(key),
                line.number,
            )

        if rest:
            result[key] = _parse_value(rest, line.number)
            i += 1
            continue

        # Valore su righe successive: mapping annidato o lista.
        if i + 1 >= len(lines):
            result[key] = None
            i += 1
            continue

        nxt = lines[i + 1]
        if nxt.indent > line.indent:
            result[key], i = _parse_block(lines, i + 1, nxt.indent)
            continue
        if nxt.indent == line.indent and (
            nxt.text == "-" or nxt.text.startswith("- ")
        ):
            # Lista allo STESSO indent della chiave: forma YAML legittima e comune.
            result[key], i = _parse_sequence(lines, i + 1, nxt.indent)
            continue
        result[key] = None
        i += 1
    return result, i


# ---------------------------------------------------------------------------
# API
# ---------------------------------------------------------------------------


def parse(source):
    """Legge un documento e ritorna dict/list/scalari. Alza `YamlError`."""
    if not isinstance(source, str):
        raise YamlError("sorgente non testuale: {!r}".format(type(source).__name__))
    lines = _tokenize(source)
    if not lines:
        return None
    base = lines[0].indent
    if base != 0:
        raise YamlError("il documento comincia indentato", lines[0].number)
    value, consumed = _parse_block(lines, 0, 0)
    if consumed != len(lines):
        raise YamlError(
            "contenuto non consumato dal parser: struttura non riconosciuta",
            lines[consumed].number,
        )
    return value


def parse_file(path):
    """Legge un file UTF-8 (BOM tollerato) e lo passa a `parse`."""
    with open(path, "r", encoding="utf-8-sig") as handle:
        return parse(handle.read())
