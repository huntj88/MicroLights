import { t } from 'i18next';

import { NameEditor } from '../../common/NameEditor';

interface PatternNameEditorProps {
  name: string;
  onChange: (name: string) => void;
}

export const PatternNameEditor = ({
  name,
  onChange,
}: PatternNameEditorProps) => {
  return (
    <NameEditor
      name={name}
      onChange={onChange}
      label={t('patternEditor.form.nameLabel')}
      placeholder={t('patternEditor.form.namePlaceholder')}
      helperText={t('patternEditor.form.nameHelper')}
    />
  );
};
