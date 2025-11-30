# Cheat Menu System

Система меню для управления читами с префиксами `ebash3d_` и `kek_`.

## Использование

### Основная команда
```
cheatmenu
```
Показывает главное меню с опциями.

### Подменю
```
cheatmenu 1          # или cheatmenu esp
cheatmenu 2          # или cheatmenu aimbot
cheatmenu 3          # или cheatmenu movement
cheatmenu 4          # или cheatmenu misc
cheatmenu 5          # или cheatmenu qt / cheatmenu quicktoggle
cheatmenu 6          # или cheatmenu list
```

### Быстрое переключение (Toggle)
```
cheatmenu toggle <cvar_name>
```
Переключает квар между 0 и 1.

Примеры:
```
cheatmenu toggle kek_esp
cheatmenu toggle kek_aimbot
cheatmenu toggle ebash3d_auto_strafe
```

### Установка значения
```
cheatmenu set <cvar_name> <value>
```
Устанавливает значение квара.

Примеры:
```
cheatmenu set kek_aimbot_fov 90
cheatmenu set kek_aimbot_smooth 0.5
cheatmenu set ebash3d_speed_multiplier 7.0
```

### Получение значения
```
cheatmenu get <cvar_name>
```
Показывает текущее значение квара.

### Быстрое переключение общих функций
```
cheatmenu qt esp          # Переключить ESP
cheatmenu qt aimbot       # Переключить Aimbot
cheatmenu qt antiaim      # Переключить Anti-Aim
cheatmenu qt speed        # Переключить Speed Boost
cheatmenu qt strafe       # Переключить Auto Strafe
cheatmenu qt glow         # Переключить Viewmodel Glow
```

## Поддерживаемые функции

### ESP (kek_esp_*)
- `kek_esp` - включить/выключить ESP
- `kek_esp_box` - показывать рамки
- `kek_esp_name` - показывать имена
- `kek_esp_weapon` - показывать оружие
- Цветовые настройки RGB для различных элементов

### Aimbot (kek_aimbot_*)
- `kek_aimbot` - включить/выключить aimbot
- `kek_aimbot_fov` - поле зрения
- `kek_aimbot_smooth` - плавность
- `kek_aimbot_visible_only` - только видимые цели
- `kek_aimbot_psilent` - perfect silent режим
- `kek_aimbot_dm` - режим deathmatch

### Anti-Aim (kek_antiaim_*)
- `kek_antiaim` - включить/выключить anti-aim
- `kek_antiaim_mode` - режим работы
- `kek_antiaim_speed` - скорость вращения

### Movement (ebash3d_*)
- `ebash3d_speed_multiplier` - множитель скорости
- `ebash3d_auto_strafe` - авто страф
- `ebash3d_ground_strafe` - земной страф
- `ebash3d_fakelag` - фейк лаг в миллисекундах
- `ebash3d_cmd_block` - блокировка команд сервера

Команды:
- `ebash3d_speed` - переключить ускорение
- `ebash3d_strafe` - переключить автостраф
- `ebash3d_gstrafe` - переключить земной страф

### ID Commands
- `ebash3d_get_id` - получить текущий ID игрока
- `ebash3d_change_id [new_id]` - изменить ID игрока

## Примеры использования

### Быстрое включение ESP и Aimbot
```
cheatmenu qt esp
cheatmenu qt aimbot
```

### Настройка Aimbot
```
cheatmenu set kek_aimbot_fov 60
cheatmenu set kek_aimbot_smooth 0.7
cheatmenu toggle kek_aimbot_psilent
```

### Настройка движения
```
cheatmenu set ebash3d_speed_multiplier 5.0
ebash3d_strafe
cheatmenu set ebash3d_fakelag 100
```

### Просмотр всех настроек
```
cheatmenu list
```

## Технические детали

- Файл: `engine/client/cheatmenu.c`
- Инициализация: автоматически при загрузке клиента через `CL_InitLocal()`
- Все команды регистрируются через `Cmd_AddCommand()`
- Система использует стандартные функции движка для работы с кварами
