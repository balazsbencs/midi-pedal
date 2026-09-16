import { useMemo, useState } from "react";

import type { Bank } from "@midi-pedal/protocol";

interface BankListProps {
  banks: Bank[];
  selected: number;
  onSelect: (index: number) => void;
}

export function BankList({ banks, selected, onSelect }: BankListProps) {
  const [query, setQuery] = useState("");
  const visible = useMemo(() => {
    const needle = query.trim().toLowerCase();
    return banks.map((bank, index) => ({ bank, index })).filter(({ bank, index }) => !needle || bank.name.toLowerCase().includes(needle) || String(index + 1).includes(needle));
  }, [banks, query]);
  return (
    <nav className="bank-pane" aria-label="Banks">
      <div className="sidebar-content">
        <div className="section-heading compact">
          <h2>Banks</h2>
          <span className="count-badge">{banks.length}</span>
        </div>
        <label className="search-label" htmlFor="bank-search">Search banks</label>
        <input id="bank-search" type="search" value={query} onChange={event => setQuery(event.target.value)} placeholder="Name or number" />
        <ul className="bank-list">
          {visible.map(({ bank, index }) => (
            <li key={bank.id}>
              <button type="button" className={`bank-row ${selected === index ? "is-selected" : ""}`} aria-current={selected === index ? "true" : undefined} onClick={() => onSelect(index)}>
                <span>{String(index + 1).padStart(3, "0")}</span><strong>{bank.name}</strong><i aria-hidden="true">›</i>
              </button>
            </li>
          ))}
          {visible.length === 0 && <li><p className="muted-copy">No banks match that search.</p></li>}
        </ul>
      </div>
    </nav>
  );
}
