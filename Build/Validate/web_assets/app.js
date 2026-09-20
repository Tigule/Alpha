(() => {
  const root = document.querySelector("[data-filter-root]");
  const list = document.querySelector("[data-filter-list]");
  if (!root || !list) return;

  const rows = [...list.querySelectorAll(".filter-row")];
  const search = root.querySelector("[data-search-input]");
  const category = root.querySelector("[data-category-filter]");
  const signature = root.querySelector("[data-signature-filter]");
  const sort = root.querySelector("[data-sort]");
  const count = document.querySelector("[data-result-count]");
  const zero = document.querySelector("[data-zero-results]");
  const number = (row, key) => Number(row.dataset[key] || 0);

  const update = () => {
    const query = (search?.value || "").trim().toLowerCase();
    const categoryValue = category?.value || "";
    const signatureValue = signature?.value || "";
    let shown = 0;

    for (const row of rows) {
      const visible =
        (!query || row.dataset.search.includes(query)) &&
        (!categoryValue || row.dataset.category === categoryValue) &&
        (!signatureValue || row.dataset.signature === signatureValue);
      row.classList.toggle("hidden", !visible);
      if (visible) shown++;
    }

    const mode = sort?.value || "";
    rows.sort((left, right) => {
      if (mode === "size-desc") return number(right, "size") - number(left, "size");
      if (mode === "percent-desc") return number(right, "percent") - number(left, "percent");
      if (mode === "percent") return number(left, "percent") - number(right, "percent");
      if (mode === "count-desc") return number(right, "count") - number(left, "count");
      if (mode === "rva") return number(left, "rva") - number(right, "rva");
      return (left.dataset.name || "").localeCompare(right.dataset.name || "");
    });

    for (const row of rows) list.append(row);
    if (count) count.textContent = `${shown.toLocaleString()} shown`;
    if (zero) zero.classList.toggle("hidden", shown !== 0);
  };

  root.addEventListener("input", update);
  root.addEventListener("change", update);
  update();
})();
