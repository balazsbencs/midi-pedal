interface EmptyStateProps { title: string; detail: string; action?: { label: string; onClick: () => void } }

export function EmptyState({ title, detail, action }: EmptyStateProps) {
  return <section className="empty-state" aria-labelledby="empty-state-title"><span className="empty-icon" aria-hidden="true">+</span><h2 id="empty-state-title">{title}</h2><p>{detail}</p>{action && <button type="button" onClick={action.onClick}>{action.label}</button>}</section>;
}

