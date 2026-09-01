export function CompatibilityError({ message, onDismiss }: { message: string; onDismiss?: () => void }) {
  return <section className="compatibility-error" role="alert"><strong>Incompatible pedal</strong><p>{message}</p>{onDismiss && <button type="button" onClick={onDismiss}>Dismiss</button>}</section>;
}

