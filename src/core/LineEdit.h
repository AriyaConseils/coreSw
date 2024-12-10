#pragma once

#include "SwWidget.h"
#include <string>
#include <algorithm> 
#include "Timer.h"

// echo mode ●●●●●●●●●●
class LineEdit : public SwWidget {

    SW_OBJECT(LineEdit, SwWidget)

    CUSTOM_PROPERTY(std::string, Text, "") {
        cursorPos = getText().size();
        selectionStart = selectionEnd = cursorPos;
        setDisplayText(getText());
    }


    CUSTOM_PROPERTY(std::string, DisplayText, "") {
        if (getEchoMode() == EchoModeEnum::PasswordEcho) {
            std::string maskedText(m_DisplayText.size(), 0x25CF);
            m_DisplayText = maskedText;
        }
        update();
    }


    CUSTOM_PROPERTY(EchoModeEnum, EchoMode, EchoModeEnum::NormalEcho) {
        if (getEchoMode() == EchoModeEnum::PasswordEcho) {
            std::string maskedText(m_DisplayText.size(), 0x25CF);
            m_DisplayText = maskedText;
        }
        else if (getEchoMode() == EchoModeEnum::NoEcho) {
            m_DisplayText = m_Text;
        }
    }

        

    CUSTOM_PROPERTY(std::string, Placeholder, "") {
        update();
    }

    CUSTOM_PROPERTY(bool, ReadOnly, false) {
        if (m_ReadOnly) {
            cursorPos = getText().size();
            selectionStart = selectionEnd = cursorPos;
            update();
        }
    }

public:
    LineEdit(const std::string& placeholderText = "", SwWidget* parent = nullptr)
        : SwWidget(parent), cursorPos(0),
          selectionStart(0), selectionEnd(0), isSelecting(false) {
        width = 300;
        height = 30;
        setCursor(CursorType::IBeam);
        setPlaceholder(placeholderText);
        connect(this, SIGNAL(TextChanged), std::function<void(std::string)>([&](std::string text) {
            setDisplayText(text);
        }));
        this->setFocusPolicy(FocusPolicyEnum::Strong);
        std::string css = R"(
            LineEdit {
                border-radius: 10px;  
            }
        )";
        this->setStyleSheet(css);

        monitorTimer = new Timer(500);
        connect(monitorTimer, SIGNAL(timeout), std::function<void()>([&]() {
            if (this->getFocus() == true) {
                update();
            }
            }));

        connect(this, SIGNAL(FocusChanged), std::function<void(bool)>([&](bool focus) {
            if (focus == true) {
                monitorTimer->start();
            }
            else {
                monitorTimer->stop();
            }
            }));
    }

    LineEdit(SwWidget* parent = nullptr)
        : SwWidget(parent), cursorPos(0),
        selectionStart(0), selectionEnd(0), isSelecting(false) {

        std::string css = R"(
            LineEdit {
                border-radius: 10px;  
            }
        )";
        this->setStyleSheet(css);


        width = 300;
        height = 30;
        setCursor(CursorType::IBeam);
        connect(this, SIGNAL(TextChanged), std::function<void(std::string)>([&](std::string text) {
            setDisplayText(text);
        }));  

        monitorTimer = new Timer(500);
        connect(monitorTimer, SIGNAL(timeout), std::function<void()>([&]() {
            if (this->getFocus() == true) {
                update();
            }
        }));

        connect(this, SIGNAL(FocusChanged), std::function<void(bool)>([&](bool focus) {
            if (focus == true) {
                monitorTimer->start();
            } else {
                monitorTimer->stop();
            }
         }));
    }


    // Redéfinir la méthode paintEvent pour dessiner le champ de texte
    virtual void paintEvent(PaintEvent* event) override {
        SwWidget::paintEvent(event);

        HDC hdc = event->context();

        // Obtenir la police via getFont()
        SwFont font = getFont();             // Supposons que getFont() renvoie un objet SwFont
        HFONT hFont = font.handle(hdc);

        // Sélectionner la police dans le HDC
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        // Style du pinceau et du stylo
        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));  // Fond blanc
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));      // Bordure noire

        // Sélectionner le pinceau et le stylo
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN hOldPen = (HPEN)SelectObject(hdc, pen);

        // Dessiner le rectangle du champ de texte
        Rectangle(hdc, x, y, x + width, y + height);

        // Définir la zone de texte
        RECT textRect;
        textRect.left = x + 5;
        textRect.top = y + 5;
        textRect.right = x + width - 5;
        textRect.bottom = y + height - 5;

        // Fond transparent pour le texte
        SetBkMode(hdc, TRANSPARENT);

        // Déterminer le texte à afficher
        const std::string& textToDraw = (getDisplayText().empty() && !getFocus()) ? getPlaceholder() : getDisplayText();

        // Dessiner le texte avec gestion de la sélection
        size_t drawPos = 0;
        int textX = x + 5;
        int textY = y + ((height - 20) / 2);

        for (size_t i = 0; i < textToDraw.length(); ++i) {
            wchar_t ch = textToDraw[i];
            SIZE charSize;
			GetTextExtentPoint32W(hdc, &ch, 1, &charSize);

            RECT charRect = { textX, y + 5, textX + charSize.cx, y + height - 5 };

            // Vérifier si le caractère est dans la sélection
			bool inSelection = (getFocus() && (selectionStart != selectionEnd) &&
								(i >= (std::min)(selectionStart, selectionEnd)) &&
								(i < (std::max)(selectionStart, selectionEnd)));

            if (inSelection) {
                // Dessiner le fond de sélection
                HBRUSH selBrush = CreateSolidBrush(RGB(0, 120, 215));  // Couleur de sélection bleue
                FillRect(hdc, &charRect, selBrush);
                DeleteObject(selBrush);
                SetTextColor(hdc, RGB(255, 255, 255));  // Texte blanc sur fond bleu
            }
            else {
                if (getText().empty()) {
                    SetTextColor(hdc, RGB(150, 150, 150));  // Texte gris
                }
                else {
                    SetTextColor(hdc, RGB(0, 0, 0));  // Texte noir
                }
            }

            // Dessiner le caractère
			ExtTextOutW(hdc, textX, textY, ETO_CLIPPED, &charRect, &ch, 1, nullptr);

            textX += charSize.cx;
        }

        // Dessiner le curseur clignotant
        if (getFocus() && GetTickCount() % 1000 < 500 && selectionStart == selectionEnd) {
            SIZE textSize;
            GetTextExtentPoint32A(hdc, textToDraw.substr(0, cursorPos).c_str(), static_cast<int>(cursorPos), &textSize);
            int cursorX = x + 5 + textSize.cx;

            MoveToEx(hdc, cursorX, y + 5, nullptr);
            LineTo(hdc, cursorX, y + height - 5);
        }

        // Restaurer les anciens objets sélectionnés dans le HDC
        SelectObject(hdc, hOldFont);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);

        // Libérer les ressources
        DeleteObject(brush);
        DeleteObject(pen);
    }


    // Gérer les événements de pression de touche
    virtual void keyPressEvent(KeyEvent* event) override {

        if (getFocus()) {
            wchar_t key = event->key();

            if (event->isCtrlPressed()) {
                // Raccourcis clavier
                if (key == 'C' || key == 'c') {  // Copier
                    copySelectionToClipboard();
                }
                else if (key == 'V' || key == 'v') {  // Coller
                    pasteFromClipboard();
                    emit TextChanged(m_Text);
                }
                else if (key == 'X' || key == 'x') {  // Couper
                    copySelectionToClipboard();
                    deleteSelection();
                    emit TextChanged(m_Text);
                }
                else if (key == 'A' || key == 'a') {  // Tout sélectionner
                    selectionStart = 0;
                    selectionEnd = m_Text.length();
                    cursorPos = m_Text.length();
                    update();
                }
            }
            else {
                if (key == VK_BACK) {  // Supprimer avant le curseur
                    if (selectionStart != selectionEnd) {
                        deleteSelection();
                        emit TextChanged(m_Text);
                    }
                    else if (cursorPos > 0) {
                        m_Text.erase(cursorPos - 1, 1);
                        cursorPos--;
                        emit TextChanged(m_Text);
                    }
                }
                else if (key == VK_DELETE) {  // Supprimer après le curseur
                    if (selectionStart != selectionEnd) {
                        deleteSelection();
                        emit TextChanged(m_Text);
                    }
                    else if (cursorPos < m_Text.length()) {
                        m_Text.erase(cursorPos, 1);
                        emit TextChanged(m_Text);
                    }
                }
                else if (key == VK_LEFT) {  // Déplacer le curseur à gauche
                    if (cursorPos > 0) {
                        cursorPos--;
                        if (event->isShiftPressed()) {
                            selectionEnd = cursorPos;
                        }
                        else {
                            selectionStart = selectionEnd = cursorPos;
                        }
                        update();
                    }
                }
                else if (key == VK_RIGHT) {  // Déplacer le curseur à droite
                    if (cursorPos < m_Text.length()) {
                        cursorPos++;
                        if (event->isShiftPressed()) {
                            selectionEnd = cursorPos;
                        }
                        else {
                            selectionStart = selectionEnd = cursorPos;
                        }
                        update();
                    }
                }
                else if (key == VK_HOME) {  // Début du texte
                    cursorPos = 0;
                    if (event->isShiftPressed()) {
                        selectionEnd = cursorPos;
                    }
                    else {
                        selectionStart = selectionEnd = cursorPos;
                    }
                    update();
                }
                else if (key == VK_END) {  // Fin du texte
                    cursorPos = m_Text.length();
                    if (event->isShiftPressed()) {
                        selectionEnd = cursorPos;
                    }
                    else {
                        selectionStart = selectionEnd = cursorPos;
                    }
                    update();
                }
                else if (iswprint(key) || (key >= '0' && key <= '9')) {  // Ajouter un caractère ou un chiffre
                    if (selectionStart != selectionEnd) {
                        deleteSelection();
                    }

                    // Vérifiez si Caps Lock est activé
                    bool capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                    wchar_t charToInsert = key;

                    // Gérer la saisie des caractères en fonction de Caps Lock
                    if (iswalpha(key)) {  // Si c'est une lettre
                        if (!capsLock) {
                            charToInsert = towlower(key);  // Convertir en minuscule si Caps Lock est actif
                        }
                        else {
                            charToInsert = towupper(key);  // Convertir en majuscule si Caps Lock est inactif
                        }
                    }

                    // Insérer le caractère
                    m_Text.insert(cursorPos, 1, charToInsert);
                    cursorPos++;
                    selectionStart = selectionEnd = cursorPos;
                    emit TextChanged(m_Text);
                }
            }

            event->accept();
        }
    }



    // Gérer le clic de souris pour le focus et le positionnement du curseur
    virtual void mousePressEvent(MouseEvent* event) override {
        if (!getReadOnly()) {
            this->setFocus(true);
            size_t clickedPos = getCharacterIndexAtPosition(event->x());
            cursorPos = clickedPos;
            if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
                selectionEnd = cursorPos;
            } else {
                selectionStart = selectionEnd = cursorPos;
            }
            isSelecting = true;
            event->accept();
        } else {
            this->setFocus(false);
            selectionStart = selectionEnd = 0;
        }
        SwWidget::mousePressEvent(event);
    }

    virtual void mouseDoubleClickEvent(MouseEvent* event) override {
        selectionStart = 0;
        selectionEnd = getText().size();
        event->accept();
        isSelecting = true;
        update();
    }

    // Gérer la sélection avec la souris
    virtual void mouseMoveEvent(MouseEvent* event) override {
        if (isSelecting) {
            size_t newPos = getCharacterIndexAtPosition(event->x());
            bool toBeUpdate = newPos != cursorPos;
            cursorPos = newPos;
            selectionEnd = cursorPos;
            event->accept();
            if(toBeUpdate) update();
        }
        if (getReadOnly()) {
            setCursor(CursorType::Arrow);
        }
        else {
            setCursor(CursorType::IBeam);
        }
        SwWidget::mouseMoveEvent(event);
    }

    // Terminer la sélection à la libération de la souris
    virtual void mouseReleaseEvent(MouseEvent* event) override {
        if (isSelecting) {
            event->accept();
        }
        isSelecting = false;
        update();
        SwWidget::mouseReleaseEvent(event);
    }




private:
    size_t cursorPos;          // Position du curseur
    size_t selectionStart;     // Début de la sélection
    size_t selectionEnd;       // Fin de la sélection
    bool isSelecting;          // Indique si une sélection est en cours
    Timer* monitorTimer;

    // Obtenir l'index du caractère à une position x
    size_t getCharacterIndexAtPosition(int xPos) {
        int relativeX = xPos - (x + 5);  // Position relative
        size_t index = 0;
        int currentX = 0;
        HDC hdc = GetDC(hwnd);

        while (index < m_Text.length()) {
            wchar_t ch = m_Text[index];
            SIZE charSize;
            GetTextExtentPoint32W(hdc, &ch, 1, &charSize);
            if (currentX + charSize.cx / 2 >= relativeX) {
                break;
            }
            currentX += charSize.cx;
            index++;
        }

        ReleaseDC(hwnd, hdc);
        return index;
    }

    // Supprimer la sélection
    void deleteSelection() {
		size_t start = (std::min)(selectionStart, selectionEnd);
		size_t end = (std::max)(selectionStart, selectionEnd);
        m_Text.erase(start, end - start);
        cursorPos = start;
        selectionStart = selectionEnd = cursorPos;
    }

    // Copier la sélection dans le presse-papiers
    void copySelectionToClipboard() {
        if (selectionStart == selectionEnd) {
            return;  // Rien à copier
        }

		size_t start = (std::min)(selectionStart, selectionEnd);
		size_t end = (std::max)(selectionStart, selectionEnd);
        std::string selectedText = (getEchoMode() == EchoModeEnum::NormalEcho)? m_Text.substr(start, end - start) : "Better luck next time, mate!";

        // Ouvrir le presse-papiers
        if (OpenClipboard(hwnd)) {
            EmptyClipboard();
            size_t sizeInBytes = (selectedText.length() + 1) * sizeof(wchar_t);
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
            memcpy(GlobalLock(hMem), selectedText.c_str(), sizeInBytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }
    }

    // Coller depuis le presse-papiers
    void pasteFromClipboard() {
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData != NULL) {
                std::string clipboardText = wcharToChar(static_cast<wchar_t*>(GlobalLock(hData)));
                if (clipboardText != "") {
                    if (selectionStart != selectionEnd) {
                        deleteSelection();
                    }
                    std::string clipboardStr = clipboardText;
                    m_Text.insert(cursorPos, clipboardStr);
                    cursorPos += clipboardStr.length();
                    selectionStart = selectionEnd = cursorPos;
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
    }

    std::string wcharToChar(const wchar_t* wideString) {
        if (!wideString) return "";

        // Taille nécessaire pour le buffer de char
        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wideString, -1, nullptr, 0, nullptr, nullptr);
        std::string result(sizeNeeded, '\0'); // Réserve la taille nécessaire
        WideCharToMultiByte(CP_ACP, 0, wideString, -1, &result[0], sizeNeeded, nullptr, nullptr);
        return result;
    }
};
