import { HSVA, Numberify, tinycolor } from '@ctrl/tinycolor';
import { ColorPicker as FluentColorPicker, ColorSlider, type ColorPickerProps as FluentColorPickerProps, ColorArea } from '@fluentui/react-color-picker-preview';
import { useState } from 'react';

interface ColorPickerProps {
    initialColor: string;
    onColorChange: (color: Numberify<HSVA>) => void;
}

export const ColorPicker: React.FC<ColorPickerProps> = ({ initialColor, onColorChange }) => {
    const DEFAULT_COLOR_HSV = tinycolor(initialColor).toHsv();
    const [color, setColor] = useState(DEFAULT_COLOR_HSV);
    const handleChange: FluentColorPickerProps['onColorChange'] = (_, data) => {
        const newColor = { ...data.color, a: data.color.a ?? 1 };
        setColor(newColor);
        onColorChange(newColor);
    };

    return (
        <>
            <FluentColorPicker color={color} onColorChange={handleChange}>
                <ColorSlider />
                <ColorArea />
            </FluentColorPicker>
        </>
    );
};