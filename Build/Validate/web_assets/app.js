(() => {
  const store = window.explorerStore || {};
  const waiting = {};
  window.explorerData = (key, value) => {
    store[key] = value;
    (waiting[key] || []).forEach((resolve) => resolve(value));
    delete waiting[key];
  };

  const load = (key, path) => {
    if (store[key]) return Promise.resolve(store[key]);
    return new Promise((resolve, reject) => {
      (waiting[key] = waiting[key] || []).push(resolve);
      if (waiting[key].length > 1) return;
      const script = document.createElement("script");
      script.src = path;
      script.onerror = () => reject(new Error(`Could not load ${path}`));
      document.head.append(script);
    });
  };

  const read = (key, fallback) => {
    try {
      return localStorage.getItem(key) ?? fallback;
    } catch (error) {
      return fallback;
    }
  };

  const write = (key, value) => {
    try {
      localStorage.setItem(key, value);
    } catch (error) {}
  };

  const h = (value) =>
    String(value ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
  const num = (value) => (typeof value === "number" ? value.toLocaleString("en-US") : "n/a");
  const pct = (part, whole) => (whole ? `${((100 * part) / whole).toFixed(2)}%` : "n/a");
  const pctValue = (value) => (typeof value === "number" ? `${value.toFixed(2)}%` : "n/a");
  const hex = (value) => (typeof value === "number" ? `0x${value.toString(16)}` : "—");

  const STATUS = {
    exact: { label: "Exact", bar: "bg-blue-900 dark:bg-blue-400", pill: "bg-blue-900 text-white dark:bg-blue-400 dark:text-slate-950" },
    address: { label: "Address-adjusted", bar: "bg-blue-500 dark:bg-blue-600", pill: "bg-blue-500 text-white dark:bg-blue-600" },
    macro: { label: "Macro-adjusted", bar: "bg-blue-300 dark:bg-blue-800", pill: "bg-blue-200 text-blue-950 dark:bg-blue-900 dark:text-blue-100" },
    differs: { label: "Differs", bar: "bg-orange-600 dark:bg-orange-500", pill: "bg-orange-600 text-white dark:bg-orange-500 dark:text-slate-950" },
    missing: { label: "Missing", bar: "bg-slate-300 dark:bg-slate-600", pill: "bg-slate-200 text-slate-800 dark:bg-slate-700 dark:text-slate-100" },
    unverified: { label: "Unverified", bar: "bg-slate-500 dark:bg-slate-400", pill: "bg-slate-500 text-white dark:bg-slate-400 dark:text-slate-950" },
    inlined: { label: "No code in either", bar: "bg-blue-200", pill: "bg-blue-50 text-blue-900 ring-1 ring-inset ring-blue-200 dark:bg-blue-950 dark:text-blue-200 dark:ring-blue-800" },
    emitted: { label: "Code in rebuild only", bar: "bg-orange-200", pill: "bg-orange-50 text-orange-900 ring-1 ring-inset ring-orange-200 dark:bg-orange-950 dark:text-orange-200 dark:ring-orange-800" },
    extra: { label: "Extra in rebuild", bar: "bg-orange-200", pill: "bg-orange-50 text-orange-900 ring-1 ring-inset ring-orange-200 dark:bg-orange-950 dark:text-orange-200 dark:ring-orange-800" },
  };
  const KIND = { class: "class", structure: "struct", union: "union", enum: "enum" };
  const ORDER = ["exact", "address", "macro", "differs", "missing", "unverified"];
  const GOOD = "bg-blue-50 text-blue-900 ring-1 ring-inset ring-blue-200 dark:bg-blue-950 dark:text-blue-200 dark:ring-blue-800";
  const BAD = "bg-orange-50 text-orange-900 ring-1 ring-inset ring-orange-200 dark:bg-orange-950 dark:text-orange-200 dark:ring-orange-800";
  const NEUTRAL = "bg-slate-100 text-slate-700 ring-1 ring-inset ring-slate-200 dark:bg-slate-800 dark:text-slate-300 dark:ring-slate-700";
  const RESULT = {
    matched: ["Same", GOOD],
    ignored: ["Ignored", NEUTRAL],
    mismatch: ["Differs", BAD],
    missing: ["Original only", BAD],
    extra: ["Rebuilt only", BAD],
    type_mismatch: ["Type differs", BAD],
    location_mismatch: ["Location differs", BAD],
    offset_mismatch: ["Offset differs", BAD],
    section_mismatch: ["Section differs", BAD],
    size_matched: ["Size matches", NEUTRAL],
  };
  const TYPE_STATUS = {
    matched: ["Matches", GOOD],
    mismatch: ["Differs", BAD],
    missing: ["Missing", NEUTRAL],
    ambiguous: ["Ambiguous", NEUTRAL],
    unsupported: ["Unverified", NEUTRAL],
  };
  const TYPE_DOT = {
    matched: "bg-blue-900 dark:bg-blue-400",
    mismatch: "bg-orange-600 dark:bg-orange-500",
    missing: "bg-slate-300 dark:bg-slate-600",
    ambiguous: "bg-slate-500",
    unsupported: "bg-slate-500",
  };

  const pill = (classes, label) => `<span class="inline-block whitespace-nowrap rounded px-2 py-0.5 text-xs font-medium ${classes}">${h(label)}</span>`;
  const statusPill = (status) => pill((STATUS[status] || STATUS.unverified).pill, (STATUS[status] || STATUS.unverified).label);
  const resultPill = (status) => pill((RESULT[status] || RESULT.mismatch)[1], (RESULT[status] || RESULT.mismatch)[0]);
  const signaturePill = (status) =>
    status === "matched" ? pill(GOOD, "Signature matches") : status === "mismatch" ? pill(BAD, "Signature differs") : pill(NEUTRAL, "Signature not compared");

  const card = "rounded-lg border border-slate-200 bg-white dark:border-slate-800 dark:bg-slate-900";
  const muted = "text-slate-600 dark:text-slate-400";
  const mono = "font-mono text-[12.5px]";
  const th = "px-4 py-2 text-left text-xs font-medium text-slate-600 dark:text-slate-400 border-b border-slate-200 dark:border-slate-800";
  const td = "px-4 py-2 align-top border-b border-slate-100 dark:border-slate-800/70";
  const segmented = (active) =>
    `min-h-8 rounded px-3 text-sm ${active ? "bg-white font-semibold text-slate-900 shadow-sm dark:bg-slate-700 dark:text-white" : "text-slate-700 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"}`;

  const table = (headers, rows, minWidth = "min-w-[640px]") =>
    `<div class="overflow-x-auto"><table class="w-full ${minWidth} border-collapse text-sm"><thead><tr>${headers
      .map((label) => `<th scope="col" class="${th}">${h(label)}</th>`)
      .join("")}</tr></thead><tbody>${rows.join("") || `<tr><td class="${td} ${muted}" colspan="${headers.length}">None</td></tr>`}</tbody></table></div>`;

  const bar = (counts, total, height = "h-3") => {
    const segments = ORDER.filter((key) => counts[key])
      .map((key) => `<span class="${STATUS[key].bar} block h-full" style="flex:${counts[key]} 1 0" title="${h(STATUS[key].label)}: ${num(counts[key])}"></span>`)
      .join("");
    return `<div class="flex ${height} overflow-hidden rounded bg-slate-200 dark:bg-slate-800">${total ? segments : ""}</div>`;
  };

  const legend = (counts, format) =>
    `<ul class="flex flex-wrap gap-x-5 gap-y-1.5 text-sm">${ORDER.filter((key) => counts[key])
      .map((key) => `<li class="flex items-center gap-2"><span class="h-2.5 w-2.5 rounded-sm ${STATUS[key].bar}" aria-hidden="true"></span>${h(STATUS[key].label)}<span class="${muted} tabular-nums">${h(format(counts[key]))}</span></li>`)
      .join("")}</ul>`;

  const summary = store.summary;
  const app = document.getElementById("app");
  const state = {
    grouping: read("grouping", "folder"),
    open: new Set(JSON.parse(read("open", "[]"))),
    treeFilter: "",
    typeFilter: "",
    typeStatus: "",
    typeKind: "",
    typePage: 0,
    codeFilter: "all",
    statusFilter: "",
    functionFilter: "",
  };

  const folderTree = () => {
    const root = { name: "", path: "", children: new Map(), compilands: [], total: 0, verified: 0 };
    Object.values(summary.compilands).forEach((entry) => {
      const parts = state.grouping === "library" ? [entry.library || "Unknown"] : entry.folders || ["Other", entry.library || "Unknown"];
      let node = root;
      node.total += entry.progress.total_bytes;
      node.verified += entry.progress.verified_bytes;
      parts.forEach((part, index) => {
        const path = `${state.grouping}:${parts.slice(0, index + 1).join("/")}`;
        if (!node.children.has(part)) node.children.set(part, { name: part, path, children: new Map(), compilands: [], total: 0, verified: 0 });
        node = node.children.get(part);
        node.total += entry.progress.total_bytes;
        node.verified += entry.progress.verified_bytes;
      });
      node.compilands.push(entry);
    });
    return root;
  };

  const progressPct = (verified, total) => (total ? (100 * verified) / total : null);

  const treeRows = (node, depth, current, query, out) => {
    const folders = [...node.children.values()].sort((a, b) => a.name.localeCompare(b.name));
    folders.forEach((child) => {
      const matches = query ? countMatches(child, query) : 1;
      if (!matches) return;
      const open = query ? true : state.open.has(child.path);
      const value = progressPct(child.verified, child.total);
      out.push(treeButton({ depth, label: child.name, value, folder: true, open, action: "toggle", target: child.path }));
      if (open) treeRows(child, depth + 1, current, query, out);
    });
    node.compilands
      .filter((entry) => !query || entry.name.toLowerCase().includes(query) || (entry.object || "").toLowerCase().includes(query))
      .sort((a, b) => a.name.localeCompare(b.name))
      .forEach((entry) => {
        out.push(treeButton({ depth, label: entry.name, value: progressPct(entry.progress.verified_bytes, entry.progress.total_bytes), href: `#/c/${entry.id}`, selected: entry.id === current }));
      });
    return out;
  };

  const countMatches = (node, query) =>
    node.compilands.filter((entry) => entry.name.toLowerCase().includes(query) || (entry.object || "").toLowerCase().includes(query)).length +
    [...node.children.values()].reduce((sum, child) => sum + countMatches(child, query), 0);

  const treeButton = ({ depth, label, value, folder, open, action, target, href, selected }) => {
    const shown = value == null ? "n/a" : `${Math.floor(value)}%`;
    const width = value == null ? 0 : Math.max(0, Math.min(100, value));
    const chevron = `<svg viewBox="0 0 12 12" class="h-3 w-3 flex-none text-slate-500 transition-transform ${open ? "rotate-90" : ""} ${folder ? "" : "invisible"}" aria-hidden="true"><path d="M4 2.5 7.5 6 4 9.5" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>`;
    const inner = `${chevron}<span class="min-w-0 flex-1 truncate text-left">${h(label)}</span><span class="h-1.5 w-9 flex-none overflow-hidden rounded bg-slate-200 dark:bg-slate-800" aria-hidden="true"><span class="block h-full bg-blue-500" style="width:${width}%"></span></span><span class="w-10 flex-none text-right text-xs tabular-nums ${muted}">${shown}</span>`;
    const classes = `flex min-h-8 w-full items-center gap-1.5 rounded pr-2 text-sm ${folder ? "font-semibold" : ""} ${selected ? "bg-blue-100 font-semibold text-blue-950 dark:bg-blue-950 dark:text-blue-100" : "hover:bg-slate-200/60 dark:hover:bg-slate-800"}`;
    const style = `padding-left:${8 + depth * 14}px`;
    if (folder) return `<li role="treeitem" aria-expanded="${open}"><button type="button" class="${classes}" style="${style}" data-action="${action}" data-target="${h(target)}">${inner}</button></li>`;
    return `<li role="treeitem" aria-selected="${!!selected}"><a class="${classes}" style="${style}" href="${href}">${inner}</a></li>`;
  };

  const sidebar = (route) => {
    if (route.area === "types") {
      const query = state.typeFilter.toLowerCase();
      const kinds = [...new Set(summary.types.map((row) => row[2]))].sort();
      const statuses = [...new Set(summary.types.map((row) => row[3]))].sort();
      const matches = summary.types.filter(
        (row) => (!query || row[1].toLowerCase().includes(query)) && (!state.typeStatus || row[3] === state.typeStatus) && (!state.typeKind || row[2] === state.typeKind),
      );
      const pages = Math.max(1, Math.ceil(matches.length / 100));
      state.typePage = Math.min(state.typePage, pages - 1);
      const shown = matches.slice(state.typePage * 100, state.typePage * 100 + 100);
      const pager = (step, label, disabled) =>
        `<button type="button" data-action="page" data-target="${step}" ${disabled ? "disabled" : ""} class="h-8 rounded-md border border-slate-300 px-2.5 text-sm font-medium hover:bg-slate-200/60 disabled:opacity-40 disabled:hover:bg-transparent dark:border-slate-700 dark:hover:bg-slate-800">${label}</button>`;
      const select = (name, label, values, current, labels) =>
        `<label class="flex flex-1 flex-col gap-0.5 text-xs ${muted}">${label}<select data-input="${name}" class="h-8 rounded-md border border-slate-300 bg-white px-1.5 text-sm text-slate-900 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"><option value="">All</option>${values
          .map((value) => `<option value="${h(value)}" ${value === current ? "selected" : ""}>${h(labels ? (labels[value] || [value])[0] : KIND[value] || value)}</option>`)
          .join("")}</select></label>`;
      return `<div class="flex flex-col gap-2.5 p-3.5">
        <label><span class="sr-only">Filter types</span><input type="search" data-input="typeFilter" value="${h(state.typeFilter)}" placeholder="Filter types" class="h-9 w-full rounded-md border border-slate-300 bg-white px-2.5 text-sm dark:border-slate-700 dark:bg-slate-900"></label>
        <div class="flex gap-2">${select("typeStatus", "Status", statuses, state.typeStatus, TYPE_STATUS)}${select("typeKind", "Kind", kinds, state.typeKind)}</div>
        <div class="flex items-center justify-between gap-2">${pager(-1, "Previous", state.typePage === 0)}<span class="text-center text-xs tabular-nums ${muted}">${num(matches.length)} types<br>Page ${num(state.typePage + 1)} of ${num(pages)}</span>${pager(1, "Next", state.typePage >= pages - 1)}</div>
      </div>
      <ul class="px-1.5 pb-4">${shown
        .map(
          (row) =>
            `<li><a href="#/t/${row[0]}" class="flex min-h-8 items-center gap-2 rounded px-2 text-sm ${route.type === row[0] ? "bg-blue-100 font-semibold dark:bg-blue-950" : "hover:bg-slate-200/60 dark:hover:bg-slate-800"}"><span class="h-2.5 w-2.5 flex-none rounded-sm ${TYPE_DOT[row[3]] || "bg-slate-500"}" aria-hidden="true"></span><span class="min-w-0 flex-1 truncate font-mono text-[12.5px]">${h(row[1])}</span><span class="text-xs ${muted}">${h(KIND[row[2]] || row[2])}</span></a></li>`,
        )
        .join("")}</ul>`;
    }
    const rows = treeRows(folderTree(), 0, route.compiland, state.treeFilter.toLowerCase(), []);
    return `<div class="flex flex-col gap-2.5 p-3.5">
      <div role="group" aria-label="Group compilands by" class="flex gap-0.5 rounded-md bg-slate-200/70 p-0.5 dark:bg-slate-800">
        <button type="button" data-action="group" data-target="folder" aria-pressed="${state.grouping === "folder"}" class="flex-1 ${segmented(state.grouping === "folder")}">Source folder</button>
        <button type="button" data-action="group" data-target="library" aria-pressed="${state.grouping === "library"}" class="flex-1 ${segmented(state.grouping === "library")}">Library</button>
      </div>
      <label><span class="sr-only">Filter compilands</span><input type="search" data-input="treeFilter" value="${h(state.treeFilter)}" placeholder="Filter compilands" class="h-9 w-full rounded-md border border-slate-300 bg-white px-2.5 text-sm dark:border-slate-700 dark:bg-slate-900"></label>
    </div>
    <ul role="tree" aria-label="Compilands" class="px-1.5 pb-4">${rows.join("")}</ul>`;
  };

  const header = (route) => {
    const dark = document.documentElement.classList.contains("dark");
    const nav = [
      ["compilands", "Compilands", "#/"],
      ["types", "Types", "#/t"],
    ]
      .map(
        ([key, label, href]) =>
          `<a href="${href}" ${route.area === key ? 'aria-current="page"' : ""} class="rounded-md px-3 py-2 text-sm ${route.area === key ? "bg-blue-50 font-semibold text-blue-900 dark:bg-blue-950 dark:text-blue-100" : "text-slate-700 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"}">${label}</a>`,
      )
      .join("");
    const commit = summary.commit
      ? `<a class="font-mono text-blue-700 hover:underline dark:text-blue-300" href="https://github.com/${h(summary.repository || "Tigule/Alpha")}/commit/${h(summary.commit)}">${h(summary.commit.slice(0, 7))}</a>`
      : "";
    const icon = dark
      ? '<svg viewBox="0 0 24 24" class="h-5 w-5" aria-hidden="true"><circle cx="12" cy="12" r="4" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>'
      : '<svg viewBox="0 0 24 24" class="h-5 w-5" aria-hidden="true"><path d="M20 14.5A8 8 0 0 1 9.5 4a8 8 0 1 0 10.5 10.5Z" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/></svg>';
    return `<header class="sticky top-0 z-10 border-b border-slate-200 bg-white/95 backdrop-blur dark:border-slate-800 dark:bg-slate-900/95">
      <div class="flex min-h-14 flex-wrap items-center gap-x-5 gap-y-2 px-5 py-2">
        <a href="#/" class="text-base font-bold">Tigule Alpha</a>
        <nav aria-label="Primary" class="flex gap-1">${nav}</nav>
        <span class="ml-auto flex flex-wrap items-center gap-x-4 text-sm ${muted}"><span class="font-medium text-slate-900 dark:text-slate-100">${h(summary.executable)}</span>${commit}</span>
        <button type="button" data-action="theme" class="grid h-10 w-10 place-items-center rounded-md text-slate-700 hover:bg-slate-100 dark:text-slate-200 dark:hover:bg-slate-800" aria-label="${dark ? "Switch to light mode" : "Switch to dark mode"}">${icon}</button>
      </div>
    </header>`;
  };

  const breadcrumb = (items) =>
    `<nav aria-label="Breadcrumb" class="flex flex-wrap items-center gap-1.5 text-sm ${muted}">${items
      .map(([label, href], index) =>
        index === items.length - 1
          ? `<span aria-current="page" class="text-slate-900 dark:text-slate-100">${h(label)}</span>`
          : `<a class="text-blue-700 hover:underline dark:text-blue-300" href="${href}">${h(label)}</a><span aria-hidden="true">/</span>`,
      )
      .join("")}</nav>`;

  const compilandCrumbs = (entry) => {
    const items = [[summary.executable, "#/"]];
    (state.grouping === "library" ? [entry.library || "Unknown"] : entry.folders || ["Other", entry.library || "Unknown"]).forEach((part) => items.push([part, "#/"]));
    items.push([entry.name, `#/c/${entry.id}`]);
    return items;
  };

  const tile = (label, value, detail) =>
    `<article class="${card} flex flex-col gap-1 p-4"><h2 class="text-sm font-medium ${muted}">${h(label)}</h2><strong class="text-2xl font-bold tracking-tight">${h(value)}</strong><span class="text-sm ${muted}">${h(detail)}</span></article>`;

  const rootView = () => {
    const m = summary.metrics;
    const verified = m.code_and_signature;
    const counts = {};
    Object.entries(summary.categories).forEach(([key, value]) => (counts[key] = (counts[key] || 0) + value.bytes));
    const tiles = [
      tile("Functions", pctValue(m.code.count_percent), `${num(m.code.matched_count)} of ${num(m.code.total_count)} match`),
      tile("Compilands", pctValue(m.compilands.count_percent), `${num(m.compilands.matched_count)} of ${num(m.compilands.total_count)} metadata matches`),
      tile("Types", pctValue(m.types.count_percent), `${num(m.types.matched_count)} of ${num(m.types.total_count)} match`),
      m.globals ? tile("Globals", pctValue(m.globals.count_percent), `${num(m.globals.matched_count)} of ${num(m.globals.total_count)} match`) : "",
      tile("Extra in rebuild", num(summary.rebuilt_only_count), "functions with no code in the original"),
    ].join("");
    const sections = (summary.sections || []).map(
      (row) =>
        `<tr><th scope="row" class="${td} text-left font-mono text-[12.5px] font-medium">${h(row.name)}</th><td class="${td} text-right tabular-nums">${num(row.original?.virtual_size)}</td><td class="${td} text-right tabular-nums">${num(row.rebuilt?.virtual_size)}</td><td class="${td}">${resultPill(row.status)}</td></tr>`,
    );
    const inputs = [];
    Object.entries(summary.inputs || {}).forEach(([side, artifacts]) =>
      Object.entries(artifacts).forEach(([kind, artifact]) =>
        inputs.push(
          `<tr><th scope="row" class="${td} whitespace-nowrap text-left font-medium">${side === "original" ? "Original" : "Rebuilt"} ${h(kind === "exe" ? "executable" : kind.toUpperCase())}</th><td class="${td} ${mono} whitespace-nowrap">${h(String(artifact.path || "").replace(/\\/g, "/").split("/").pop())}</td><td class="${td} ${mono}" title="${h(artifact.sha256)}">${h(String(artifact.sha256 || "").slice(0, 16))}</td><td class="${td} text-right tabular-nums">${num(artifact.size)}</td></tr>`,
        ),
      ),
    );
    const identity = summary.identity || {};
    const verifiedText = ["original", "rebuilt"]
      .map((side) => `${side === "original" ? "Original" : "Rebuilt"}: ${identity[side]?.verified ? "verified" : "not verified"}`)
      .join(", ");
    return `<section class="flex flex-col gap-4">
      <h1 class="text-2xl font-bold">${h(summary.executable)}</h1>
      <div class="${card} flex flex-col gap-3 p-4">
        <div class="flex flex-wrap items-baseline gap-x-3.5 gap-y-1"><strong class="text-3xl font-bold tracking-tight">${pctValue(verified.byte_percent)}</strong><span class="${muted}">${num(verified.matched_bytes)} of ${num(verified.total_bytes)} code bytes match with signature</span></div>
        ${bar(counts, verified.total_bytes, "h-3.5")}
        ${legend(counts, (value) => pct(value, verified.total_bytes))}
      </div>
      <div class="grid grid-cols-2 gap-3 md:grid-cols-3 xl:grid-cols-5">${tiles}</div>
      <div class="${card}"><h2 class="px-4 py-3.5 text-base font-semibold">Sections</h2>${table(["Section", "Original bytes", "Rebuilt bytes", "Result"], sections, "min-w-[480px]")}</div>
      <div class="${card}"><div class="flex flex-wrap items-baseline justify-between gap-2 px-4 py-3.5"><h2 class="text-base font-semibold">Inputs</h2><span class="text-sm ${muted}">${h(verifiedText)}</span></div>${table(["Input", "File", "SHA-256", "Bytes"], inputs, "min-w-[600px]")}</div>
    </section>`;
  };

  const compilandView = (data, route) => {
    const p = data.progress;
    const functions = data.functions.map((row, index) => ({ ...row, index }));
    const codeCount = functions.filter((row) => row.kind === "code").length;
    const inlineCount = functions.length - codeCount;
    const tab = route.tab || "functions";
    const tabs = [
      ["functions", "Functions", functions.length],
      ["headers", "Headers", data.headers.length],
      ["data", "Data", data.globals.length],
      ["metadata", "Metadata", data.metadata.length ? (data.metadata_status === "matched" ? "Matches" : "Differs") : "—"],
    ]
      .map(
        ([key, label, count]) =>
          `<a role="tab" aria-selected="${tab === key}" href="#/c/${data.id}/${key}" class="-mb-px flex min-h-10 items-center gap-1.5 border-b-2 px-3.5 text-sm ${tab === key ? "border-blue-600 font-semibold text-slate-900 dark:border-blue-400 dark:text-white" : "border-transparent font-medium text-slate-700 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"}">${label}<span class="font-normal tabular-nums ${muted}">${h(count)}</span></a>`,
      )
      .join("");
    let body = "";
    if (tab === "functions") {
      const query = state.functionFilter.toLowerCase();
      const statusCounts = {};
      functions.forEach((row) => (statusCounts[row.status] = (statusCounts[row.status] || 0) + 1));
      const shown = functions.filter(
        (row) =>
          (state.codeFilter === "all" || (state.codeFilter === "code" ? row.kind === "code" : row.kind === "inline")) &&
          (!state.statusFilter || row.status === state.statusFilter) &&
          (!query || String(row.name).toLowerCase().includes(query)),
      );
      const codeButtons = [
        ["all", `All ${functions.length}`],
        ["code", `With code ${codeCount}`],
        ["inline", `No code ${inlineCount}`],
      ]
        .map(([key, label]) => `<button type="button" data-action="code" data-target="${key}" aria-pressed="${state.codeFilter === key}" class="${segmented(state.codeFilter === key)}">${label}</button>`)
        .join("");
      const statusOptions = Object.keys(STATUS)
        .filter((key) => statusCounts[key])
        .map((key) => `<option value="${key}" ${state.statusFilter === key ? "selected" : ""}>${h(STATUS[key].label)} (${statusCounts[key]})</option>`)
        .join("");
      const rows = shown.map(
        (row) =>
          `<tr class="hover:bg-slate-50 dark:hover:bg-slate-800/50"><td class="${td}"><a class="${mono} text-blue-700 hover:underline dark:text-blue-300 break-all" href="#/c/${data.id}/f/${row.index}">${h(row.name)}</a></td><td class="${td}">${row.kind === "code" ? pill("bg-slate-900 text-white dark:bg-slate-100 dark:text-slate-900", "Code") : pill(NEUTRAL, "No code")}</td><td class="${td}">${statusPill(row.status)}</td><td class="${td} ${row.signature_status === "mismatch" ? "text-orange-800 dark:text-orange-300" : muted}">${row.kind === "code" && row.status !== "extra" ? (row.signature_status === "matched" ? "Matches" : row.signature_status === "mismatch" ? "Differs" : "Not compared") : ""}</td><td class="${td} text-right tabular-nums ${muted}">${row.size != null ? `${num(row.size)} B` : row.rebuilt_size != null ? `${num(row.rebuilt_size)} B rebuilt` : ""}</td></tr>`,
      );
      body = `<div class="flex flex-col gap-2.5">
        <div class="flex flex-wrap items-center gap-2">
          <div role="group" aria-label="Code" class="flex gap-0.5 rounded-md bg-slate-200/70 p-0.5 dark:bg-slate-800">${codeButtons}</div>
          <label class="flex items-center gap-2 text-sm ${muted}">Status<select data-input="statusFilter" class="h-9 rounded-md border border-slate-300 bg-white px-2 text-sm text-slate-900 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-100"><option value="">All</option>${statusOptions}</select></label>
          <label class="ml-auto"><span class="sr-only">Filter functions</span><input type="search" data-input="functionFilter" value="${h(state.functionFilter)}" placeholder="Filter functions" class="h-9 w-56 rounded-md border border-slate-300 bg-white px-2.5 text-sm dark:border-slate-700 dark:bg-slate-900"></label>
        </div>
        <div class="${card}">${table(["Function", "Code", "Status", "Signature", "Size"], rows, "min-w-[720px]")}</div>
      </div>`;
    } else if (tab === "headers") {
      body = `<div class="${card}">${table(
        ["Header", "Original", "Rebuilt", "Result"],
        data.headers.map(
          (row) =>
            `<tr><td class="${td} ${mono} font-medium">${h(row.name)}</td><td class="${td} ${mono} break-all ${muted}">${h(row.original || "—")}</td><td class="${td} ${mono} break-all ${muted}">${h(row.rebuilt || "—")}</td><td class="${td}">${resultPill(row.status)}</td></tr>`,
        ),
      )}</div>`;
    } else if (tab === "data") {
      body = `<div class="${card}">${table(
        ["Name", "Type", "Scope", "Section", "Result"],
        data.globals.map((row) => {
          const value = row.original || row.rebuilt || {};
          const type = row.status === "type_mismatch" ? `${row.original.type} / ${row.rebuilt.type}` : value.type;
          const section = row.status === "section_mismatch" ? `${row.original.section} / ${row.rebuilt.section}` : value.section;
          return `<tr><td class="${td} ${mono} break-all">${h(row.name)}</td><td class="${td} ${mono}">${h(type)}</td><td class="${td}">${value.scope === "global" ? "Global" : "File static"}</td><td class="${td} ${mono}">${h(section)}</td><td class="${td}">${resultPill(row.status)}</td></tr>`;
        }),
      )}</div>`;
    } else {
      const display = (value) => (value == null || value === "" ? "—" : typeof value === "object" ? JSON.stringify(value) : value);
      body = `<div class="flex flex-col gap-2">${data.metadata_reason ? `<p class="text-sm ${muted}">${h(data.metadata_reason)}</p>` : ""}<div class="${card}">${table(
        ["Field", "Original", "Rebuilt", "Result"],
        data.metadata.map(
          (row) =>
            `<tr><th scope="row" class="${td} text-left font-medium">${h(row.field)}</th><td class="${td} ${mono} break-all">${h(display(row.original))}</td><td class="${td} ${mono} break-all">${h(display(row.rebuilt))}</td><td class="${td}">${resultPill(row.status)}</td></tr>`,
        ),
      )}</div></div>`;
    }
    const linkage = data.linkage.map(
      (row) =>
        `<tr><th scope="row" class="${td} text-left font-medium">${h(row.field)}</th><td class="${td} ${mono} break-all">${h(row.original ?? "—")}</td><td class="${td} ${mono} break-all">${h(row.rebuilt ?? "—")}</td><td class="${td}">${resultPill(row.status)}</td></tr>`,
    );
    return `<section class="flex flex-col gap-4">
      ${breadcrumb(compilandCrumbs(data))}
      <div class="flex flex-col gap-1"><h1 class="text-2xl font-bold">${h(data.name)}</h1><span class="${mono} ${muted} break-all">${h(data.path || data.identity)}</span></div>
      <div class="${card} flex flex-col gap-2.5 p-4">
        <div class="flex flex-wrap items-baseline gap-x-3.5 gap-y-1"><strong class="text-2xl font-bold">${pct(p.verified_bytes, p.total_bytes)}</strong><span class="${muted}">${num(p.verified_bytes)} of ${num(p.total_bytes)} code bytes match with signature. ${num(p.code_count)} of ${num(p.total_count)} functions with code match.</span></div>
        ${bar(p.statuses, p.total_count)}
        ${legend(p.statuses, (value) => num(value))}
      </div>
      ${linkage.length ? `<div class="${card}">${table(["Linkage", "Original", "Rebuilt", "Result"], linkage)}</div>` : ""}
      <div role="tablist" aria-label="Compiland sections" class="flex flex-wrap gap-0.5 border-b border-slate-200 dark:border-slate-800">${tabs}</div>
      ${body}
    </section>`;
  };

  const disassembly = (evidence) => {
    if (!evidence || !Array.isArray(evidence.rows)) return "";
    const cell = (record, status, side) => {
      const empty = (status === "inserted" && side === "original") || (status === "deleted" && side === "rebuilt");
      const tone = status === "context" ? muted : empty ? "bg-slate-50 dark:bg-slate-800/40" : "bg-orange-50 text-slate-900 dark:bg-orange-950/60 dark:text-orange-50";
      const text = record ? (record.mnemonic ? `${record.mnemonic}${record.operands ? ` ${record.operands}` : ""}` : record.text || record.directive || "") : "";
      return `<td class="whitespace-pre px-3 py-0.5 ${tone}">${h(text)}</td>`;
    };
    const rows = evidence.rows
      .map((row) => {
        const status = row.status || "context";
        const record = row.original || row.rebuilt || {};
        const offset = typeof record.function_offset === "number" ? `+${hex(record.function_offset)}` : "";
        return `<tr class="border-t border-slate-100 dark:border-slate-800 ${row.focus ? "outline outline-2 -outline-offset-2 outline-orange-500" : ""}" title="${h(row.note || "")}"><td class="px-4 py-0.5 ${muted}">${h(offset)}</td>${cell(row.original, status, "original")}${cell(row.rebuilt, status, "rebuilt")}</tr>`;
      })
      .join("");
    const omitted = [evidence.omitted_before ? `${num(evidence.omitted_before)} rows hidden before` : "", evidence.omitted_after ? `${num(evidence.omitted_after)} rows hidden after` : ""].filter(Boolean).join(", ");
    return `<div class="overflow-x-auto border-t border-slate-100 dark:border-slate-800"><table class="w-full min-w-[600px] border-collapse font-mono text-[12.5px]"><thead><tr class="text-left font-sans text-xs ${muted}"><th scope="col" class="w-20 px-4 py-1.5 font-medium">Offset</th><th scope="col" class="px-3 py-1.5 font-medium">Original</th><th scope="col" class="px-3 py-1.5 font-medium">Rebuilt</th></tr></thead><tbody>${rows}</tbody></table></div>${omitted ? `<p class="px-4 py-2 text-xs ${muted}">${h(omitted)}</p>` : ""}`;
  };

  const functionView = (data, route) => {
    const row = data.functions[route.fn];
    if (!row) return `<p class="${muted}">Function not found.</p>`;
    const locals = row.locals || [];
    const localDiff = locals.filter((value) => value.status !== "matched").length;
    const localRows = locals.map(
      (value) =>
        `<tr><td class="${td} ${mono}">${h(value.name)}</td><td class="${td} ${mono}">${h((value.original || value.rebuilt).type)}${value.status === "type_mismatch" ? ` / ${h(value.rebuilt.type)}` : ""}</td><td class="${td} ${mono}">${h(value.original?.location ?? "—")}</td><td class="${td} ${mono}">${h(value.rebuilt?.location ?? "—")}</td><td class="${td}">${resultPill(value.status)}</td></tr>`,
    );
    const notes = {
      exact: "Bytes identical.",
      address: "Identical after normalizing relocated addresses.",
      macro: "Identical after normalizing __FILE__ and __LINE__ values.",
      missing: "The rebuilt binary has no function with this identity.",
      unverified: row.reason || "This function could not be compared.",
      inlined: "Neither binary has out-of-line code for this method. Every caller inlined it, or nothing uses it.",
      emitted: "The original has no out-of-line code for this function, but the rebuild emits one.",
      extra: "The rebuild has a function with no counterpart in the original binary.",
    };
    const sizes = [];
    if (row.rva != null) sizes.push(`Original RVA ${hex(row.rva)}${row.size != null ? `, ${num(row.size)} B` : ""}`);
    if (row.rebuilt_rva != null) sizes.push(`Rebuilt RVA ${hex(row.rebuilt_rva)}${row.rebuilt_size != null ? `, ${num(row.rebuilt_size)} B` : ""}`);
    const code =
      row.status === "differs" && row.disassembly
        ? `<div class="flex flex-wrap items-baseline justify-between gap-2 px-4 pb-3"><span class="text-sm ${muted}">${h(row.reason || "")}</span><span class="text-sm ${muted}">Original ${num(row.disassembly.original_instruction_count)} instructions, rebuilt ${num(row.disassembly.rebuilt_instruction_count)}</span></div>${disassembly(row.disassembly)}`
        : `<p class="border-t border-slate-100 px-4 py-3 text-sm dark:border-slate-800">${h(notes[row.status] || row.reason || "")}${row.raw?.equal && row.raw.original?.sha256 ? `<span class="mt-1 block ${mono} break-all ${muted}">SHA-256 ${h(row.raw.original.sha256)}</span>` : ""}</p>`;
    const signatureStatus = row.status === "extra" ? null : row.kind === "inline" ? (row.rebuilt_signature ? (row.rebuilt_signature === row.original_signature ? "matched" : "mismatch") : null) : row.signature_status;
    return `<section class="flex flex-col gap-4">
      ${breadcrumb([...compilandCrumbs(data), [row.name, ""]])}
      <div class="flex flex-col gap-2">
        <h1 class="break-all font-mono text-xl font-medium">${h(row.name)}</h1>
        <div class="flex flex-wrap items-center gap-1.5">${statusPill(row.status)}</div>
        ${sizes.length ? `<div class="flex flex-wrap gap-x-6 ${mono} ${muted}">${sizes.map((value) => `<span>${h(value)}</span>`).join("")}</div>` : ""}
      </div>
      <div class="${card} flex flex-col gap-2.5 p-4">
        <div class="flex flex-wrap items-center justify-between gap-2"><h2 class="text-base font-semibold">Signature</h2>${signatureStatus ? signaturePill(signatureStatus) : ""}</div>
        <div class="grid grid-cols-[repeat(auto-fit,minmax(260px,1fr))] gap-2 ${mono}">
          <div class="flex flex-col gap-1 rounded-md bg-slate-50 px-3 py-2.5 dark:bg-slate-800/60"><span class="font-sans text-xs ${muted}">Original</span><span class="break-words">${h(row.original_signature || "—")}</span></div>
          <div class="flex flex-col gap-1 rounded-md bg-slate-50 px-3 py-2.5 dark:bg-slate-800/60"><span class="font-sans text-xs ${muted}">Rebuilt</span><span class="break-words">${h(row.rebuilt_signature || "—")}</span></div>
        </div>
        ${signatureStatus === "mismatch" && row.signature_reason ? `<p class="text-sm ${muted}">${h(row.signature_reason)}</p>` : ""}
      </div>
      ${row.kind === "code" ? `<div class="${card}"><div class="flex flex-wrap items-center justify-between gap-2 px-4 py-3.5"><h2 class="text-base font-semibold">Locals</h2>${locals.length ? pill(localDiff ? BAD : GOOD, localDiff ? `${localDiff} of ${locals.length} differ` : `All ${locals.length} match`) : ""}</div>${table(["Name", "Type", "Original", "Rebuilt", "Result"], localRows)}</div>` : ""}
      <div class="${card}"><h2 class="px-4 py-3.5 text-base font-semibold">Code</h2>${code}</div>
      ${row.audit && row.audit.length ? `<details class="text-sm"><summary class="flex min-h-8 cursor-pointer items-center font-semibold">Normalization audit</summary><pre class="mt-2 overflow-x-auto rounded-md bg-slate-100 p-3 font-mono text-xs dark:bg-slate-800">${h(JSON.stringify(row.audit, null, 2))}</pre></details>` : ""}
    </section>`;
  };

  const typeView = (data) => {
    const [label, tone] = TYPE_STATUS[data.status] || TYPE_STATUS.unsupported;
    const members = data.members || [];
    const methods = data.methods || [];
    const memberDiff = members.filter((row) => row.status !== "matched").length;
    const noCode = methods.filter((row) => !row.original_code).length;
    const size = (value) => (value == null ? "—" : hex(value));
    const facts = [
      ["Size", size(data.original_size), size(data.rebuilt_size)],
      ["Members", members.filter((row) => row.original != null).length, members.filter((row) => row.rebuilt != null).length],
    ].map(([field, left, right]) => `<tr><th scope="row" class="${td} text-left font-medium">${field}</th><td class="${td} ${mono}">${h(left)}</td><td class="${td} ${mono}">${h(right)}</td><td class="${td}">${resultPill(String(left) === String(right) ? "matched" : "mismatch")}</td></tr>`);
    const memberRows = members.map(
      (row) =>
        `<tr><td class="${td} ${mono} ${muted}">${row.offset == null ? "" : `+${hex(row.offset)}`}</td><td class="${td} ${mono}">${h(row.kind === "base_class" ? "base" : row.name)}</td><td class="${td} ${mono}">${h(row.original ?? "—")}</td><td class="${td} ${mono}">${h(row.rebuilt ?? "—")}${row.status === "offset_mismatch" ? ` at +${hex(row.rebuilt_offset)}` : ""}</td><td class="${td}">${resultPill(row.status)}</td></tr>`,
    );
    const methodRows = methods.map(
      (row) =>
        `<tr><td class="${td} ${mono} break-all">${h(row.signature || row.name)}</td><td class="${td} ${muted}">${h([row.access, row.property, row.compiler_generated ? "compiler generated" : ""].filter(Boolean).join(", "))}</td><td class="${td}">${row.original_code ? pill("bg-slate-900 text-white dark:bg-slate-100 dark:text-slate-900", "Code") : pill(NEUTRAL, "No code")}</td><td class="${td}">${row.original_code === row.rebuilt_code ? pill(GOOD, "Same") : pill(BAD, row.rebuilt_code ? "Rebuild has code" : "Rebuild has no code")}</td></tr>`,
    );
    return `<section class="flex flex-col gap-4">
      ${breadcrumb([[summary.executable, "#/"], ["Types", "#/t"], [data.name, ""]])}
      <div class="flex flex-col gap-2">
        <h1 class="break-all font-mono text-xl font-medium">${h(data.name)}</h1>
        <div class="flex flex-wrap items-center gap-1.5">${pill(NEUTRAL, KIND[data.kind] || data.kind)}${pill(tone, label)}</div>
        ${data.reason ? `<p class="text-sm ${muted}" title="${h(data.reason)}">${h(data.reason.split(": ").pop())}</p>` : ""}
      </div>
      <div class="${card}">${table(["Field", "Original", "Rebuilt", "Result"], facts, "min-w-[480px]")}</div>
      <div class="${card}"><div class="flex flex-wrap items-center justify-between gap-2 px-4 py-3.5"><h2 class="text-base font-semibold">Members</h2>${members.length ? pill(memberDiff ? BAD : GOOD, memberDiff ? `${memberDiff} of ${members.length} differ` : `All ${members.length} match`) : ""}</div>${table(["Offset", "Name", "Original", "Rebuilt", "Result"], memberRows)}</div>
      ${data.kind === "enum" ? "" : `<div class="${card}"><div class="flex flex-wrap items-center justify-between gap-2 px-4 py-3.5"><h2 class="text-base font-semibold">Methods</h2><span class="text-sm ${muted}">${num(methods.length)} declared, ${num(noCode)} with no code in the original</span></div>${table(["Method", "Attributes", "Original", "Rebuild"], methodRows, "min-w-[720px]")}</div>`}
    </section>`;
  };

  const parseRoute = () => {
    const parts = location.hash.replace(/^#\/?/, "").split("/").filter(Boolean);
    if (parts[0] === "t") return { area: "types", type: parts[1] || null };
    if (parts[0] === "c" && parts[1]) {
      if (parts[2] === "f") return { area: "compilands", compiland: parts[1], fn: Number(parts[3]) };
      return { area: "compilands", compiland: parts[1], tab: parts[2] };
    }
    return { area: "compilands" };
  };

  let lastCompiland = null;

  const render = async () => {
    const route = parseRoute();
    if (route.compiland !== lastCompiland) {
      state.functionFilter = "";
      state.statusFilter = "";
      state.codeFilter = "all";
      lastCompiland = route.compiland;
    }
    if (route.compiland && summary.compilands[route.compiland]) {
      const entry = summary.compilands[route.compiland];
      const parts = state.grouping === "library" ? [entry.library || "Unknown"] : entry.folders || ["Other", entry.library || "Unknown"];
      parts.forEach((_, index) => state.open.add(`${state.grouping}:${parts.slice(0, index + 1).join("/")}`));
    }
    let main = "";
    try {
      if (route.area === "types") {
        const id = route.type || (summary.types[0] || [])[0];
        main = id ? typeView(await load(`type:${id}`, `data/types/${parseInt(id.slice(-10, -8), 16) % 64}.js`)) : `<p class="${muted}">No types.</p>`;
        route.type = id;
      } else if (route.compiland) {
        const data = await load(`compiland:${route.compiland}`, `data/compilands/${parseInt(route.compiland.slice(-10, -8), 16) % 128}.js`);
        main = Number.isInteger(route.fn) ? functionView(data, route) : compilandView(data, route);
      } else {
        main = rootView();
      }
    } catch (error) {
      main = `<p class="${muted}">${h(error.message)}</p>`;
    }
    const focus = document.activeElement?.dataset?.input;
    const caret = focus ? document.activeElement.selectionStart : null;
    const scroll = document.querySelector("[data-sidebar]")?.scrollTop || 0;
    app.innerHTML = `${header(route)}<div class="flex flex-wrap items-stretch">
      <aside aria-label="Navigator" class="w-full border-slate-200 bg-slate-100/60 lg:w-80 lg:flex-none lg:border-r dark:border-slate-800 dark:bg-slate-900/40"><div data-sidebar class="lg:sticky lg:top-14 lg:h-[calc(100vh-3.5rem)] lg:overflow-y-auto">${sidebar(route)}</div></aside>
      <main class="min-w-0 flex-[999_1_640px] px-4 pb-10 pt-5 sm:px-6">${main}</main>
    </div>`;
    const sidebarElement = document.querySelector("[data-sidebar]");
    if (sidebarElement) sidebarElement.scrollTop = scroll;
    if (focus) {
      const element = document.querySelector(`[data-input="${focus}"]`);
      if (element) {
        element.focus();
        if (caret != null && element.setSelectionRange) element.setSelectionRange(caret, caret);
      }
    }
  };

  app.addEventListener("click", (event) => {
    const target = event.target.closest("[data-action]");
    if (!target) return;
    const action = target.dataset.action;
    if (action === "toggle") {
      state.open.has(target.dataset.target) ? state.open.delete(target.dataset.target) : state.open.add(target.dataset.target);
      write("open", JSON.stringify([...state.open]));
    } else if (action === "group") {
      state.grouping = target.dataset.target;
      write("grouping", state.grouping);
    } else if (action === "page") {
      state.typePage += Number(target.dataset.target);
    } else if (action === "code") {
      state.codeFilter = target.dataset.target;
    } else if (action === "theme") {
      const dark = !document.documentElement.classList.contains("dark");
      document.documentElement.classList.toggle("dark", dark);
      write("theme", dark ? "dark" : "light");
    }
    render();
  });

  app.addEventListener("input", (event) => {
    const name = event.target.dataset.input;
    if (!name) return;
    state[name] = event.target.value;
    if (name.startsWith("type")) state.typePage = 0;
    render();
  });

  matchMedia("(prefers-color-scheme: dark)").addEventListener("change", (event) => {
    if (read("theme", null)) return;
    document.documentElement.classList.toggle("dark", event.matches);
    render();
  });

  window.addEventListener("hashchange", () => {
    render();
    window.scrollTo(0, 0);
  });
  render();
})();
