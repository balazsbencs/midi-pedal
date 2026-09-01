import type { ValidationError } from "@midi-pedal/protocol";

export function ErrorSummary({ errors }: { errors: ValidationError[] }) {
  if (!errors.length) return null;
  return <div className="error-summary" role="alert" aria-labelledby="error-summary-title"><strong id="error-summary-title">Fix these validation errors before syncing</strong><ul>{errors.slice(0, 8).map(error => <li key={`${error.path}-${error.code}`}>{error.path || "/"}: {error.message}</li>)}</ul>{errors.length > 8 && <p>and {errors.length - 8} more…</p>}</div>;
}

