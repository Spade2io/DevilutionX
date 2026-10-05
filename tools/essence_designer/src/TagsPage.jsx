import { useState } from 'react'
import { AddBox, NameInput, newId, sameName } from './parts.jsx'

// Data shapes used on this page:
//   group: { id, name, exclusive }
//   tag:   { id, name, groupId, parentId }   parentId is null for a top-level tag
// The order of the lists is the order shown on screen.

export default function TagsPage({ data, update }) {
  const { tagGroups, tags } = data
  const [selectedTag, setSelectedTag] = useState(null)
  const [dragOver, setDragOver] = useState(null)

  const nameTaken = (name, exceptId) => tags.some((t) => t.id !== exceptId && sameName(t.name, name))

  const addGroup = (name) =>
    update((d) => ({ ...d, tagGroups: [...d.tagGroups, { id: newId('g'), name, exclusive: false }] }))

  const changeGroup = (id, fields) =>
    update((d) => ({ ...d, tagGroups: d.tagGroups.map((g) => (g.id === id ? { ...g, ...fields } : g)) }))

  const deleteGroup = (id) =>
    update((d) => (d.tags.some((t) => t.groupId === id) ? d : { ...d, tagGroups: d.tagGroups.filter((g) => g.id !== id) }))

  const addTag = (groupId, name) =>
    update((d) => ({ ...d, tags: [...d.tags, { id: newId('t'), name, groupId, parentId: null }] }))

  const changeTag = (id, fields) =>
    update((d) => ({ ...d, tags: d.tags.map((t) => (t.id === id ? { ...t, ...fields } : t)) }))

  // Deleting a parent keeps its children; they become top-level tags.
  const deleteTag = (id) => {
    setSelectedTag(null)
    update((d) => ({
      ...d,
      tags: d.tags.filter((t) => t.id !== id).map((t) => (t.parentId === id ? { ...t, parentId: null } : t)),
    }))
  }

  // Drag and drop. Dropping on a group puts the tag at the end of that group.
  // Dropping on another tag puts it just before that tag, at the same level.
  // A parent always brings its children along.
  const moveTag = (tagId, groupId, beforeTagId) =>
    update((d) => {
      const tag = d.tags.find((t) => t.id === tagId)
      const target = beforeTagId ? d.tags.find((t) => t.id === beforeTagId) : null
      if (!tag || tagId === beforeTagId) return d
      const children = d.tags.filter((t) => t.parentId === tagId)
      let parentId = target ? target.parentId : null
      if (parentId === tagId) return d
      if (children.length > 0 && parentId) {
        // A tag that has children cannot itself become a child.
        beforeTagId = parentId
        parentId = null
      }
      const rest = d.tags.filter((t) => t.id !== tagId && t.parentId !== tagId)
      let index = beforeTagId ? rest.findIndex((t) => t.id === beforeTagId) : rest.length
      if (index < 0) index = rest.length
      rest.splice(index, 0, { ...tag, groupId, parentId }, ...children.map((c) => ({ ...c, groupId })))
      return { ...d, tags: rest }
    })

  const dropHandlers = (key, groupId, beforeTagId) => ({
    onDragOver: (e) => {
      e.preventDefault()
      e.stopPropagation()
      setDragOver(key)
    },
    onDragLeave: () => setDragOver((current) => (current === key ? null : current)),
    onDrop: (e) => {
      e.preventDefault()
      e.stopPropagation()
      setDragOver(null)
      const tagId = e.dataTransfer.getData('text/plain')
      if (tagId) moveTag(tagId, groupId, beforeTagId)
    },
  })

  const renderTag = (tag, isChild) => {
    const children = tags.filter((t) => t.parentId === tag.id)
    return (
      <li key={tag.id} className={isChild ? 'tag-row child' : 'tag-row'}>
        <button
          className={'tag' + (selectedTag === tag.id ? ' selected' : '') + (dragOver === tag.id ? ' drop-before' : '')}
          draggable
          onDragStart={(e) => e.dataTransfer.setData('text/plain', tag.id)}
          onClick={() => setSelectedTag(selectedTag === tag.id ? null : tag.id)}
          {...dropHandlers(tag.id, tag.groupId, tag.id)}
        >
          {tag.name}
        </button>
        {selectedTag === tag.id && (
          <TagEditor
            tag={tag}
            hasChildren={children.length > 0}
            parentChoices={tags.filter((t) => t.groupId === tag.groupId && !t.parentId && t.id !== tag.id)}
            nameTaken={nameTaken}
            onChange={(fields) => changeTag(tag.id, fields)}
            onDelete={() => deleteTag(tag.id)}
          />
        )}
        {children.length > 0 && <ul className="tag-list">{children.map((c) => renderTag(c, true))}</ul>}
      </li>
    )
  }

  return (
    <>
      <header className="page-head">
        <h2>Tags</h2>
        <AddBox placeholder="New group name" button="Add group" onAdd={addGroup}
          check={(name) => (tagGroups.some((g) => sameName(g.name, name)) ? 'A group with that name already exists.' : '')} />
      </header>
      <p className="hint">
        Drag a tag onto a group to move it there, or onto another tag to place it just before that tag.
        Click a tag to rename it, give it a parent, or delete it.
      </p>
      {tagGroups.length === 0 && <p className="empty">No tag groups yet. Add one above to get started.</p>}
      <div className="groups">
        {tagGroups.map((group) => {
          const topLevel = tags.filter((t) => t.groupId === group.id && !t.parentId)
          const isEmpty = !tags.some((t) => t.groupId === group.id)
          return (
            <section
              key={group.id}
              className={dragOver === group.id ? 'group drop-into' : 'group'}
              {...dropHandlers(group.id, group.id, null)}
            >
              <div className="group-head">
                <NameInput className="group-name" value={group.name}
                  check={(name) => (tagGroups.some((g) => g.id !== group.id && sameName(g.name, name)) ? 'taken' : '')}
                  onCommit={(name) => changeGroup(group.id, { name })} />
                <button className="quiet" disabled={!isEmpty} onClick={() => deleteGroup(group.id)}
                  title={isEmpty ? 'Delete this group' : 'Only an empty group can be deleted. Move or delete its tags first.'}>
                  Delete
                </button>
              </div>
              <label className="exclusive">
                <input type="checkbox" checked={group.exclusive}
                  onChange={(e) => changeGroup(group.id, { exclusive: e.target.checked })} />
                Mutually exclusive
                <span className="note">{group.exclusive ? 'a power may hold only one' : 'a power may hold several'}</span>
              </label>
              <ul className="tag-list">{topLevel.map((t) => renderTag(t, false))}</ul>
              <AddBox placeholder="New tag" button="Add" small onAdd={(name) => addTag(group.id, name)}
                check={(name) => (nameTaken(name) ? 'A tag with that name already exists.' : '')} />
            </section>
          )
        })}
      </div>
    </>
  )
}

function TagEditor({ tag, hasChildren, parentChoices, nameTaken, onChange, onDelete }) {
  const [confirming, setConfirming] = useState(false)
  return (
    <div className="tag-editor">
      <label>
        Name
        <NameInput value={tag.name} check={(name) => (nameTaken(name, tag.id) ? 'taken' : '')}
          onCommit={(name) => onChange({ name })} />
      </label>
      <label>
        Parent
        <select value={tag.parentId ?? ''} disabled={hasChildren}
          onChange={(e) => onChange({ parentId: e.target.value || null })}>
          <option value="">None (top level)</option>
          {parentChoices.map((p) => <option key={p.id} value={p.id}>{p.name}</option>)}
        </select>
      </label>
      {hasChildren && <span className="note">This tag has children, so it cannot have a parent itself.</span>}
      {confirming
        ? <button className="danger" onClick={onDelete}>Really delete "{tag.name}"?</button>
        : <button className="quiet" onClick={() => setConfirming(true)}>Delete tag</button>}
    </div>
  )
}
