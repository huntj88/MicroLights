import { tinycolor } from '@ctrl/tinycolor';
import { makeStyles, tokens } from '@fluentui/react-components';
import { ColorPicker as FluentColorPicker, ColorSlider, AlphaSlider, type ColorPickerProps as FluentColorPickerProps, ColorArea } from '@fluentui/react-color-picker-preview';
import { useState } from 'react';

const useStyles = makeStyles({
    previewColor: {
        width: '50px',
        height: '50px',
        borderRadius: tokens.borderRadiusMedium,
        border: `1px solid ${tokens.colorNeutralStroke1}`,
        margin: `${tokens.spacingVerticalMNudge} 0`,
    },
});

interface ColorPickerProps {
    initialColor: string;
    onColorChange: (color: string) => void;
}

export const ColorPicker: React.FC<ColorPickerProps> = ({ initialColor, onColorChange }) => {
    const styles = useStyles();
    const DEFAULT_COLOR_HSV = tinycolor(initialColor).toHsv();
    const [color, setColor] = useState(DEFAULT_COLOR_HSV);
    const handleChange: FluentColorPickerProps['onColorChange'] = (_, data) => {
        const newColor = { ...data.color, a: data.color.a ?? 1 };
        setColor(newColor);
        onColorChange(tinycolor(newColor).toHexString());
    };

    return (
        <>
            <FluentColorPicker color={color} onColorChange={handleChange}>
                <ColorSlider />
                <ColorArea />
            </FluentColorPicker>

            <div className={styles.previewColor} style={{ backgroundColor: tinycolor(color).toRgbString() }} />
        </>
    );
};