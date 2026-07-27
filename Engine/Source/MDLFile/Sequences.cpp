#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "GenObject.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>
#include <storm.h>

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void IAnimAddErrors(TSet &errors) {
  errors.Add(0x160, 1, 0);
  errors.Add(0x174, 0, 0);
  errors.Add(0x17A, 0, 0);
  errors.Add(0x170, 0, 0);
  errors.Add(0x16F, 0, 0);
  errors.Add(0x134, 0, 0);
  errors.Add(0x14E, 0, 0);
  errors.Add(0x1AB, 0, 0);
  errors.Add(0x130, 0, 0);
}

static void IReadVertex(Parser &parse, NTempest::C3Vector *vertex) {
  parse.Expect('{');
  vertex->x = parse.ExpectFloat();
  parse.Expect(',');
  vertex->y = parse.ExpectFloat();
  parse.Expect(',');
  vertex->z = parse.ExpectFloat();
  parse.Expect('}');
}

static void IGeosetBoundsAddErrors(TSet &errors) {
  errors.Add(0x170, 0, 0);
  errors.Add(0x16F, 0, 0);
  errors.Add(0x134, 0, 0);
}

static void ISkipGeosetBounds(Parser &parse, CMDLStatus *status) {
  CMdlBounds bounds;
  TSet errors;
  parse.Expect('{');
  IGeosetBoundsAddErrors(errors);
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (token == 0x170) {
      IReadVertex(parse, &bounds.extent.b);
    } else if (token == 0x16F) {
      IReadVertex(parse, &bounds.extent.t);
    } else if (token == 0x134) {
      bounds.radius = parse.ExpectFloat();
    } else {
      parse.FatalUnexpected(tokenText);
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

static void ReadIntRange(Parser &parse, NTempest::CiRange *range) {
  parse.Expect('{');
  range->l = parse.ExpectInt();
  parse.Expect(',');
  range->h = parse.ExpectInt();
  parse.Expect('}');
}

static void IReadAnim(
    const MDLDATA &data,
    Parser &parse,
    MDLSEQUENCESSECTION *sequence,
    CMDLStatus *status
) {
  TSet errors;
  IAnimAddErrors(errors);
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case 0x10A:
        ISkipGeosetBounds(parse, status);
        token = parse.Token(&tokenText, 0);
        continue;
      case 0x130:
        sequence->blendTime = parse.ExpectInt();
        break;
      case 0x134:
        sequence->bounds.radius = parse.ExpectFloat();
        break;
      case 0x14E:
        sequence->frequency = parse.ExpectFloat();
        break;
      case 0x160:
        ReadIntRange(parse, &sequence->time);
        break;
      case 0x1AB:
        ReadIntRange(parse, &sequence->replay);
        break;
      case 0x16F:
        IReadVertex(parse, &sequence->bounds.extent.t);
        break;
      case 0x170:
        IReadVertex(parse, &sequence->bounds.extent.b);
        break;
      case 0x174:
        sequence->movespeed = parse.ExpectFloat();
        break;
      case 0x17A:
        sequence->flags |= 1;
        break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  if (errors.NotFound(0x130) && data.version < 900) {
    sequence->blendTime = data.model.blendTime;
  }
  errors.Complete(status);
}

int ReadSequences(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int token;
  const char *tokenText;
  UTokenData value;
  long expected = parse.GetOptionalInt(&token, &tokenText, 0);
  if (expected > 0) {
    data.sequences.Reserve(expected);
  }
  parse.Expect('{', token, tokenText);
  long actual = 0;
  token = parse.Token(&tokenText, &value);
  while (token == 0x122) {
    const char *name = parse.ExpectString();
    MDLSEQUENCESSECTION *sequence = data.sequences.New();
    SStrCopy(sequence->name, name, 80);
    parse.Expect('{');
    IReadAnim(data, parse, sequence, status);
    ++actual;
    token = parse.Token(&tokenText, &value);
  }
  parse.Expect('}', token, tokenText);
  if (expected >= 0 && actual != expected) {
    parse.WarningCount("sequences", expected, actual);
  }
  return !parse.FoundError();
}

static void IWriteSequenceFlags(
    TSGrowableArray<char> &buffer,
    const MDLSEQUENCESSECTION &sequence
) {
  if (sequence.flags & 1) {
    WriteLine(buffer, "\t\t%s,\n", TokenText(0x17A));
  }
}

static void IWriteSequence(
    const MDLSEQUENCESSECTION &sequence,
    TSGrowableArray<char> &buffer
) {
  WriteLine(
      buffer,
      "\t%s \"%s\" {\n",
      TokenText(0x122),
      static_cast<const char *>(sequence.name)
  );
  WriteLine(
      buffer,
      "\t\t%s { %u, %u },\n",
      TokenText(0x160),
      sequence.time.l,
      sequence.time.h
  );
  IWriteSequenceFlags(buffer, sequence);
  WriteOptionalFloat(0x174, "\t\t", sequence.movespeed, buffer);
  WriteOptionalFloat(0x14E, "\t\t", sequence.frequency, buffer);
  if (sequence.replay.l && sequence.replay.h) {
    WriteLine(
        buffer,
        "\t\t%s { %u, %u },\n",
        TokenText(0x1AB),
        sequence.replay.l,
        sequence.replay.h
    );
  }
  if (sequence.blendTime) {
    WriteLine(
        buffer,
        "\t\t%s %u,\n",
        TokenText(0x130),
        sequence.blendTime
    );
  }
  WriteBounds(sequence.bounds, "\t\t", buffer);
  WriteLine(buffer, "\t}\n");
}

int WriteSequences(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.sequences.Count()) {
    WriteLine(
        buffer,
        "%s %d {\n",
        TokenText(0x105),
        data.sequences.Count()
    );
    for (unsigned int i = 0; i < data.sequences.Count(); ++i) {
      IWriteSequence(data.sequences.Ptr()[i], buffer);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

static unsigned int IReadOldGlobalSeqs(
    Parser &parse,
    MDLDATA &data
) {
  unsigned int actual = 0;
  unsigned int token;
  const char *tokenText;
  UTokenData value;
  do {
    MDLGLOBALSEQSECTION *sequence = data.globalSeqs.New();
    parse.Expect('{');
    long start = parse.ExpectInt();
    parse.Expect(',');
    sequence->length = parse.ExpectInt() - start;
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, &value);
  } while (token == '{');
  parse.Expect('}', token, tokenText);
  return actual;
}

static unsigned int IReadGlobalSeqs(
    Parser &parse,
    MDLDATA &data
) {
  unsigned int actual = 0;
  unsigned int token;
  const char *tokenText;
  UTokenData value;
  do {
    MDLGLOBALSEQSECTION *sequence = data.globalSeqs.New();
    sequence->length = parse.ExpectInt();
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, &value);
  } while (token == 0x143);
  parse.Expect('}', token, tokenText);
  return actual;
}

int ReadGlobalSequences(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *
) {
  unsigned int token;
  const char *tokenText;
  UTokenData value;
  long count = parse.GetOptionalInt(&token, &tokenText, 0);
  if (count > 0) {
    data.globalSeqs.Reserve(count);
  }
  parse.Expect('{', token, tokenText);
  token = parse.Token(&tokenText, &value);
  unsigned int actual =
      token == 0x143
      ? IReadGlobalSeqs(parse, data)
      : IReadOldGlobalSeqs(parse, data);
  if (count >= 0 && actual != static_cast<unsigned int>(count)) {
    parse.WarningCount("global sequences", count, actual);
  }
  return !parse.FoundError();
}

int WriteGlobalSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!static_cast<const char *>(data.model.animationFile)[0] && data.globalSeqs.Count()) {
    WriteLine(buffer, "%s %d {\n", TokenText(0x106), data.globalSeqs.Count());
    for (unsigned int i = 0; i < data.globalSeqs.Count(); ++i) {
      WriteLine(buffer, "\t%s %u,\n", TokenText(0x143), data.globalSeqs[i].length);
    }
    WriteLine(buffer, "}\n");
  }
  return 1;
}

int ReadBinGlobalSequences(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  if (length & 3) {
    status->Add(STATUS_ERROR, "Invalid GLBS section detected in model.\n");
    return 0;
  }
  unsigned int count = length / 4;
  data.globalSeqs.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    data.globalSeqs[i].length = buffer.GetUint();
  }
  return 1;
}

int WriteBinGlobalSequences(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *) {
  if (!static_cast<const char *>(data.model.animationFile)[0] && data.globalSeqs.Count()) {
    buffer.AddDword('SBLG');
    buffer.AddUint(4 * data.globalSeqs.Count());
    for (unsigned int i = 0; i < data.globalSeqs.Count(); ++i) {
      buffer.AddUint(data.globalSeqs[i].length);
    }
  }
  return 1;
}

int ReadBinSequences(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  unsigned int count = buffer.GetUint();
  if (length - 4 != 140 * count) {
    status->Add(
        STATUS_ERROR,
        "Invalid SEQX section detected in model -- nonintegral number of sequences.\n"
    );
    return 0;
  }

  data.sequences.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    MDLSEQUENCESSECTION &sequence = data.sequences.Ptr()[i];
    buffer.GetTcharArray(sequence.name, 80);
    sequence.time.l = buffer.GetInt();
    sequence.time.h = buffer.GetInt();
    sequence.movespeed = buffer.GetFloat();
    sequence.flags = buffer.GetUint();
    sequence.bounds.radius = buffer.GetFloat();
    buffer.GetFloatArray(&sequence.bounds.extent.b.x, 3);
    buffer.GetFloatArray(&sequence.bounds.extent.t.x, 3);
    sequence.frequency = buffer.GetFloat();
    sequence.replay.l = buffer.GetInt();
    sequence.replay.h = buffer.GetInt();
    sequence.blendTime = buffer.GetUint();
  }
  return 1;
}

int WriteBinSequences(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.sequences.Count()) {
    buffer.AddDword('SQES');
    buffer.AddUint(140 * data.sequences.Count() + 4);
    buffer.AddUint(data.sequences.Count());
    for (unsigned int i = 0; i < data.sequences.Count(); ++i) {
      const MDLSEQUENCESSECTION &sequence = data.sequences.Ptr()[i];
      buffer.AddTcharArray(sequence.name, 80, 1);
      buffer.AddInt(sequence.time.l);
      buffer.AddInt(sequence.time.h);
      buffer.AddFloat(sequence.movespeed);
      buffer.AddUint(sequence.flags);
      buffer.AddFloat(sequence.bounds.radius);
      buffer.AddFloatArray(&sequence.bounds.extent.b.x, 3);
      buffer.AddFloatArray(&sequence.bounds.extent.t.x, 3);
      buffer.AddFloat(sequence.frequency);
      buffer.AddInt(sequence.replay.l);
      buffer.AddInt(sequence.replay.h);
      buffer.AddUint(sequence.blendTime);
    }
  }
  return 1;
}

}
