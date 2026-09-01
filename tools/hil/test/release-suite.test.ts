import { describe, expect, it } from "vitest";

import { requiredReleaseCaseIds, releaseSuite } from "../src/cases";

describe("release suite registry", () => {
  it("contains every release-blocking acceptance case exactly once", () => {
    const ids = releaseSuite.map(item => item.id);
    expect(new Set(ids).size).toBe(ids.length);
    for (const id of requiredReleaseCaseIds) expect(ids).toContain(id);
  });
});

