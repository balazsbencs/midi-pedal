import { describe, expect, it } from "vitest";

import { calculateNextVersion, parseConventionalCommit, renderChangelog } from "./next.mjs";

describe("conventional release metadata", () => {
  it("parses scoped and breaking conventional commits", () => {
    expect(parseConventionalCommit("feat(protocol)!: negotiate protocol versions", "", "123456789")).toMatchObject({
      type: "feat",
      scope: "protocol",
      description: "negotiate protocol versions",
      breaking: true,
      hash: "123456789"
    });
    expect(parseConventionalCommit("fix(protocol): reject incompatible images", "BREAKING CHANGE: image format changed")).toMatchObject({ breaking: true });
    expect(parseConventionalCommit("not a conventional commit", "")).toBeNull();
  });

  it("selects major, minor, and patch bumps", () => {
    expect(calculateNextVersion("1.2.3", [{ type: "feat", breaking: false }])).toBe("1.3.0");
    expect(calculateNextVersion("1.2.3", [{ type: "fix", breaking: false }])).toBe("1.2.4");
    expect(calculateNextVersion("1.2.3", [{ type: "docs", breaking: true }])).toBe("2.0.0");
  });

  it("renders grouped notes with scopes and commit ids", () => {
    const notes = renderChangelog("1.3.0", [
      { type: "feat", scope: "editor", description: "add preset search", breaking: false, hash: "abcdef123456" },
      { type: "fix", scope: "firmware", description: "keep relays open at boot", breaking: false, hash: "123456789abc" }
    ], "v1.2.3");
    expect(notes).toContain("# v1.3.0");
    expect(notes).toContain("Changes since v1.2.3:");
    expect(notes).toContain("## Features");
    expect(notes).toContain("- **editor:** add preset search ([abcdef1])");
    expect(notes).toContain("## Bug Fixes");
  });
});
