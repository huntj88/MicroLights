"use client"
import { useState, useEffect, Dispatch, SetStateAction } from "react";

export function useLocalStorage<T>(key: string, initialValue: T): [T, Dispatch<SetStateAction<T>>] {
    const storedValue = localStorage.getItem(key);
    const initial = storedValue ? (JSON.parse(storedValue) as T) : initialValue;
    const [value, setValue] = useState<T>(initial);

    useEffect(() => {
        localStorage.setItem(key, JSON.stringify(value));
    }, [key, value]);

    return [value, setValue];
}