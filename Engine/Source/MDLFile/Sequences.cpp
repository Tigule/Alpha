#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "GenObject.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>
#include <storm.h>

MDLSEQUENCESSECTION::MDLSEQUENCESSECTION() : time(0), movespeed(0.0f), flags(0), bounds(), frequency(0.0f), replay(0), blendTime(150) {
  name[0] = 0;
  bounds.radius = 0.0f;
}

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  int          ReadSequences(Parser &parse, MDLDATA &data, CMDLStatus *status);
  int          WriteSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status);
  int          ReadGlobalSequences(Parser &parse, MDLDATA &data, CMDLStatus *status);
  int          WriteGlobalSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status);
  int          ReadBinGlobalSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status);
  int          WriteBinGlobalSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status);
  int          ReadBinSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status);
  int          WriteBinSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status);
}  // namespace MDL

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
  TSet       errors;
  parse.Expect('{');
  IGeosetBoundsAddErrors(errors);
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (token == 0x170) {
      IReadVertex(parse, &bounds.extent.b);
    } else if (token == 0x16F) {
      IReadVertex(parse, &bounds.extent.t);
    } else if (token == 0x134) {
      bounds.radius = parse.ExpectFloat();
    } else {
      parse.FatalUnexpected(tokentext);
    }
    parse.Expect(',');
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  errors.Complete(status);
}

static void ReadIntRange(Parser &parse, NTempest::CiRange *range) {
  parse.Expect('{');
  range->l = parse.ExpectInt();
  parse.Expect(',');
  range->h = parse.ExpectInt();
  parse.Expect('}');
}

static void IReadAnim(const MDLDATA &data, Parser &parse, MDLSEQUENCESSECTION *sequence, CMDLStatus *status) {
  TSet errors;
  IAnimAddErrors(errors);
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case 0x10A:
        ISkipGeosetBounds(parse, status);
        token = parse.Token(&tokentext, 0);
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
        parse.FatalUnexpected(tokentext);
        break;
    }
    parse.Expect(',');
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
  if (errors.NotFound(0x130) && data.version < 900) {
    sequence->blendTime = data.model.blendTime;
  }
  errors.Complete(status);
}

int MDL::ReadSequences(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData value;
  long       actual = 0;
  long       count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    data.sequences.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &value);
  while (savedtoken == 0x122) {
    LPCSTR               animname = parse.ExpectString();
    MDLSEQUENCESSECTION *sequence = data.sequences.New();
    SStrCopy(sequence->name, animname, 80);
    parse.Expect('{');
    IReadAnim(data, parse, sequence, status);
    ++actual;
    savedtoken = parse.Token(&tokentext, &value);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("sequences", count, actual);
  }
  return !parse.FoundError();
}

static void IWriteSequenceFlags(TSGrowableArray<char> &buffer, const MDLSEQUENCESSECTION &seq) {
  if (seq.flags & 1) {
    MDL::WriteLine(buffer, "\t\t%s,\n", MDL::TokenText(0x17A));
  }
}

static void IWriteSequence(const MDLSEQUENCESSECTION &times, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s \"%s\" {\n", MDL::TokenText(0x122), static_cast<LPCSTR>(times.name));
  MDL::WriteLine(buffer, "\t\t%s { %u, %u },\n", MDL::TokenText(0x160), times.time.l, times.time.h);
  IWriteSequenceFlags(buffer, times);
  WriteOptionalFloat(0x174, "\t\t", times.movespeed, buffer);
  WriteOptionalFloat(0x14E, "\t\t", times.frequency, buffer);
  if (times.replay.l && times.replay.h) {
    MDL::WriteLine(buffer, "\t\t%s { %u, %u },\n", MDL::TokenText(0x1AB), times.replay.l, times.replay.h);
  }
  if (times.blendTime) {
    MDL::WriteLine(buffer, "\t\t%s %u,\n", MDL::TokenText(0x130), times.blendTime);
  }
  WriteBounds(times.bounds, "\t\t", buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

int MDL::WriteSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.sequences.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(0x105), data.sequences.Count());
    for (UINT i = 0; i < data.sequences.Count(); ++i) {
      IWriteSequence(data.sequences.Ptr()[i], buffer);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

static UINT IReadOldGlobalSeqs(Parser &parse, MDLDATA &data) {
  UINT       actual = 0;
  UINT       token;
  LPCSTR     tokentext;
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
    token = parse.Token(&tokentext, &value);
  } while (token == '{');
  parse.Expect('}', token, tokentext);
  return actual;
}

static UINT IReadGlobalSeqs(Parser &parse, MDLDATA &data) {
  UINT actual;
  actual = 0;
  UINT       token;
  LPCSTR     tokentext;
  UTokenData value;
  do {
    MDLGLOBALSEQSECTION *sequence = data.globalSeqs.New();
    sequence->length = parse.ExpectInt();
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokentext, &value);
  } while (token == 0x143);
  parse.Expect('}', token, tokentext);
  return actual;
}

int MDL::ReadGlobalSequences(Parser &parse, MDLDATA &data, CMDLStatus *) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData value;
  long       count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    data.globalSeqs.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &value);
  UINT actual = savedtoken == 0x143 ? IReadGlobalSeqs(parse, data) : IReadOldGlobalSeqs(parse, data);
  if (count >= 0 && actual != static_cast<UINT>(count)) {
    parse.WarningCount("global sequences", count, actual);
  }
  return !parse.FoundError();
}

int MDL::WriteGlobalSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.globalSeqs.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(0x106), data.globalSeqs.Count());
    for (UINT i = 0; i < data.globalSeqs.Count(); ++i) {
      MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(0x143), data.globalSeqs[i].length);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

int MDL::ReadBinGlobalSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  FATALASSERT(status);
  if (length & 3) {
    status->Add(STATUS_ERROR, "Invalid GLBS section detected in model.\n");
    return 0;
  }
  UINT count = length / 4;
  data.globalSeqs.SetCount(count);
  for (UINT i = 0; i < count; ++i) {
    data.globalSeqs[i].length = buf.GetUint();
  }
  return 1;
}

int MDL::WriteBinGlobalSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.globalSeqs.Count()) {
    buf.AddDword('SBLG');
    buf.AddUint(4 * data.globalSeqs.Count());
    for (UINT i = 0; i < data.globalSeqs.Count(); ++i) {
      buf.AddUint(data.globalSeqs[i].length);
    }
  }
  return 1;
}

int MDL::ReadBinSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  FATALASSERT(status);
  UINT numSeqs;
  numSeqs = buf.GetUint();
  if (length - 4 != 140 * numSeqs) {
    status->Add(STATUS_ERROR, "Invalid SEQX section detected in model -- nonintegral number of sequences.\n");
    return 0;
  }

  data.sequences.SetCount(numSeqs);
  for (UINT i = 0; i < numSeqs; ++i) {
    MDLSEQUENCESSECTION &sequence = data.sequences.Ptr()[i];
    buf.GetTcharArray(sequence.name, 80);
    sequence.time.l = buf.GetInt();
    sequence.time.h = buf.GetInt();
    sequence.movespeed = buf.GetFloat();
    sequence.flags = buf.GetUint();
    sequence.bounds.radius = buf.GetFloat();
    buf.GetFloatArray(&sequence.bounds.extent.b.x, 3);
    buf.GetFloatArray(&sequence.bounds.extent.t.x, 3);
    sequence.frequency = buf.GetFloat();
    sequence.replay.l = buf.GetInt();
    sequence.replay.h = buf.GetInt();
    sequence.blendTime = buf.GetUint();
  }
  return 1;
}

int MDL::WriteBinSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numSequences = data.sequences.Count();
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && numSequences) {
    buf.AddDword('SQES');
    buf.AddUint(140 * numSequences + 4);
    buf.AddUint(numSequences);
    for (UINT i = 0; i < numSequences; ++i) {
      buf.AddTcharArray(data.sequences.Ptr()[i].name, 80, 1);
      buf.AddInt(data.sequences.Ptr()[i].time.l);
      buf.AddInt(data.sequences[i].time.h);
      buf.AddFloat(data.sequences[i].movespeed);
      buf.AddUint(data.sequences[i].flags);
      buf.AddFloat(data.sequences[i].bounds.radius);
      buf.AddFloatArray(&data.sequences[i].bounds.extent.b.x, 3);
      buf.AddFloatArray(&data.sequences[i].bounds.extent.t.x, 3);
      buf.AddFloat(data.sequences[i].frequency);
      buf.AddInt(data.sequences[i].replay.l);
      buf.AddInt(data.sequences[i].replay.h);
      buf.AddUint(data.sequences[i].blendTime);
    }
  }
  return 1;
}
