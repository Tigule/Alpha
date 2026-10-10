#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "GenObject.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <stpl.h>
#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadSequences(Parser &parse, MDLDATA &data, CMDLStatus *status);
  BOOL         WriteSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status);
  BOOL         ReadGlobalSequences(Parser &parse, MDLDATA &data, CMDLStatus *status);
  BOOL         WriteGlobalSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status);
  BOOL         ReadBinGlobalSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status);
  BOOL         WriteBinGlobalSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status);
  BOOL         ReadBinSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status);
  BOOL         WriteBinSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status);
}  // namespace MDL

static void IAnimAddErrors(TSet &errors) {
  errors.Add(MDLTOK_INTERVAL, 1, 0);
  errors.Add(MDLTOK_MOVESPEED, 0, 0);
  errors.Add(MDLTOK_NONLOOPING, 0, 0);
  errors.Add(MDLTOK_MINIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_MAXIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_BOUNDS_RADIUS, 0, 0);
  errors.Add(MDLTOK_FREQUENCY, 0, 0);
  errors.Add(MDLTOK_REPLAY, 0, 0);
  errors.Add(MDLTOK_BLEND_TIME, 0, 0);
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
  errors.Add(MDLTOK_MINIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_MAXIMUMEXTENT, 0, 0);
  errors.Add(MDLTOK_BOUNDS_RADIUS, 0, 0);
}

static void ISkipGeosetBounds(Parser &parse, CMDLStatus *status) {
  CMdlBounds bounds;
  parse.Expect('{');
  TSet   errors;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  IGeosetBoundsAddErrors(errors);
  while (token != '}' && token) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_MINIMUMEXTENT:
        IReadVertex(parse, &bounds.extent.b);
        break;
      case MDLTOK_MAXIMUMEXTENT:
        IReadVertex(parse, &bounds.extent.t);
        break;
      case MDLTOK_BOUNDS_RADIUS:
        bounds.radius = parse.ExpectFloat();
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
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

static void IReadAnim(const MDLDATA &data, Parser &parse, MDLSEQUENCESSECTION *newseq, CMDLStatus *status) {
  TSet   errors;
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  IAnimAddErrors(errors);
  for (; token != '}' && token; token = parse.Token(&tokentext, 0)) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_INTERVAL:
        ReadIntRange(parse, &newseq->time);
        break;
      case MDLTOK_MOVESPEED:
        newseq->movespeed = parse.ExpectFloat();
        break;
      case MDLTOK_NONLOOPING:
        newseq->flags |= 1;
        break;
      case MDLTOK_FREQUENCY:
        newseq->frequency = parse.ExpectFloat();
        break;
      case MDLTOK_REPLAY:
        ReadIntRange(parse, &newseq->replay);
        break;
      case MDLTOK_MINIMUMEXTENT:
        IReadVertex(parse, &newseq->bounds.extent.b);
        break;
      case MDLTOK_MAXIMUMEXTENT:
        IReadVertex(parse, &newseq->bounds.extent.t);
        break;
      case MDLTOK_BOUNDS_RADIUS:
        newseq->bounds.radius = parse.ExpectFloat();
        break;
      case MDLTOK_GEOSET:
        ISkipGeosetBounds(parse, status);
        continue;
      case MDLTOK_BLEND_TIME:
        newseq->blendTime = parse.ExpectInt();
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
    parse.Expect(',');
  }
  parse.Expect('}', token, tokentext);
  if (errors.NotFound(MDLTOK_BLEND_TIME) && data.version < 900) {
    newseq->blendTime = data.model.blendTime;
  }
  errors.Complete(status);
}

static void IWriteSequenceFlags(TSGrowableArray<char> &buffer, const MDLSEQUENCESSECTION &seq) {
  if (seq.flags & 1) {
    MDL::WriteLine(buffer, "\t\t%s,\n", MDL::TokenText(MDLTOK_NONLOOPING));
  }
}

static UINT IReadOldGlobalSeqs(Parser &parse, MDLDATA &data) {
  UINT       token;
  LPCSTR     tokentext;
  UTokenData value;
  UINT       actual = 0;
  do {
    MDLGLOBALSEQSECTION *sequence = data.globalSeqs.New();
    parse.Expect('{');
    long start = parse.ExpectInt();
    parse.Expect(',');
    sequence->length = parse.ExpectInt() - start;
    parse.Expect('}');
    parse.Expect(',');
    ++actual;
  } while ((token = parse.Token(&tokentext, &value)) == MDLTOK_INTERVAL);
  parse.Expect('}', token, tokentext);
  return actual;
}

static UINT IReadGlobalSeqs(Parser &parse, MDLDATA &data) {
  UINT       token;
  LPCSTR     tokentext;
  UTokenData value;
  UINT       actual = 0;
  do {
    MDLGLOBALSEQSECTION *sequence = data.globalSeqs.New();
    sequence->length = parse.ExpectInt();
    parse.Expect(',');
    ++actual;
  } while ((token = parse.Token(&tokentext, &value)) == MDLTOK_DURATION);
  parse.Expect('}', token, tokentext);
  return actual;
}

static void IWriteSequence(const MDLSEQUENCESSECTION &times, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "\t%s \"%s\" {\n", MDL::TokenText(MDLTOK_ANIM), (LPCSTR)times.name);
  MDL::WriteLine(buffer, "\t\t%s { %u, %u },\n", MDL::TokenText(MDLTOK_INTERVAL), times.time.l, times.time.h);
  IWriteSequenceFlags(buffer, times);
  WriteOptionalFloat(MDLTOK_MOVESPEED, "\t\t", times.movespeed, buffer);
  WriteOptionalFloat(MDLTOK_FREQUENCY, "\t\t", times.frequency, buffer);
  if (times.replay.l && times.replay.h) {
    MDL::WriteLine(buffer, "\t\t%s { %u, %u },\n", MDL::TokenText(MDLTOK_REPLAY), times.replay.l, times.replay.h);
  }
  if (times.blendTime) {
    MDL::WriteLine(buffer, "\t\t%s %u,\n", MDL::TokenText(MDLTOK_BLEND_TIME), times.blendTime);
  }
  WriteBounds(times.bounds, "\t\t", buffer);
  MDL::WriteLine(buffer, "\t}\n");
}

BOOL MDL::ReadSequences(Parser &parse, MDLDATA &data, CMDLStatus *status) {
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
  while (savedtoken == MDLTOK_ANIM) {
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

BOOL MDL::WriteSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!data.model.animationFile[0] && data.sequences.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(MDLTOK_SEQUENCES), data.sequences.Count());
    const MDLSEQUENCESSECTION *sequence = data.sequences.Ptr();
    for (UINT i = data.sequences.Count(); i; --i, ++sequence) {
      IWriteSequence(*sequence, buffer);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::ReadGlobalSequences(Parser &parse, MDLDATA &data, CMDLStatus *) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData value;
  long       count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0) {
    data.sequences.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &value);
  UINT actual = savedtoken == MDLTOK_DURATION ? IReadGlobalSeqs(parse, data) : IReadOldGlobalSeqs(parse, data);
  if (count >= 0 && actual != (UINT)count) {
    parse.WarningCount("global sequences", count, actual);
  }
  return !parse.FoundError();
}

BOOL MDL::WriteGlobalSequences(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (!data.model.animationFile[0] && data.globalSeqs.Count()) {
    MDL::WriteLine(buffer, "%s %d {\n", MDL::TokenText(MDLTOK_GLOBALSEQUENCES), data.globalSeqs.Count());
    const MDLGLOBALSEQSECTION *sequence = data.globalSeqs.Ptr();
    for (UINT i = data.globalSeqs.Count(); i; --i, ++sequence) {
      MDL::WriteLine(buffer, "\t%s %u,\n", MDL::TokenText(MDLTOK_DURATION), sequence->length);
    }
    MDL::WriteLine(buffer, "}\n");
  }
  return 1;
}

BOOL MDL::ReadBinSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(status != 0);
  VALIDATEEND;
  UINT numSeqs;
  numSeqs = buf.GetUint();
  if (length - 4 != sizeof(MDLSEQUENCESSECTION) * numSeqs) {
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

BOOL MDL::WriteBinSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numSequences = data.sequences.Count();
  if (!data.model.animationFile[0] && numSequences) {
    buf.AddDword('SQES');
    buf.AddUint(sizeof(MDLSEQUENCESSECTION) * numSequences + 4);
    buf.AddUint(numSequences);
    for (UINT i = 0; i < numSequences; ++i) {
      buf.AddTcharArray(data.sequences[i].name, 80, 1);
      buf.AddInt(data.sequences[i].time.l);
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

BOOL MDL::ReadBinGlobalSequences(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(status != 0);
  VALIDATEEND;
  if (length & 3) {
    status->Add(STATUS_ERROR, "Invalid GLBX section detected in model -- nonintegral number of global sequences.\n");
    return 0;
  }
  UINT count = length / 4;
  data.globalSeqs.SetCount(count);
  for (UINT i = 0; i < count; ++i) {
    data.globalSeqs[i].length = buf.GetUint();
  }
  return 1;
}

BOOL MDL::WriteBinGlobalSequences(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numGlobalSeqs = data.globalSeqs.Count();
  if (data.model.animationFile[0] || !numGlobalSeqs) {
    return 1;
  }
  buf.AddDword('SBLG');
  buf.AddUint(4 * numGlobalSeqs);
  for (UINT i = 0; i < numGlobalSeqs; ++i) {
    buf.AddUint(data.globalSeqs[i].length);
  }
  return 1;
}
