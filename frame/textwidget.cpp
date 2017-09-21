#include "textwidget.h"

#include <QLabel>
#include <QHBoxLayout>

#include "widgets/bigcolorbutton.h"
#include "widgets/seperatorline.h"
#include "widgets/textfontlabel.h"

TextWidget::TextWidget(QWidget *parent)
    : QWidget(parent)
{
    BigColorButton* fillBtn = new BigColorButton(this);
    connect(fillBtn, &BigColorButton::clicked, this, [=]{
        showColorPanel(DrawStatus::Fill, cursor().pos());
    });

    QLabel* colBtnLabel = new QLabel(this);
    colBtnLabel->setText(tr("Fill"));

    SeperatorLine* textSeperatorLine = new SeperatorLine(this);

    QLabel* fontsizeLabel = new QLabel(this);
    fontsizeLabel->setText(tr("Font size"));
    TextFontLabel* fontLabel = new TextFontLabel(this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(6);
    layout->addStretch();
    layout->addWidget(fillBtn);
    layout->addWidget(colBtnLabel);
    layout->addWidget(textSeperatorLine);
    layout->addWidget(fontsizeLabel);
    layout->addWidget(fontLabel);
    layout->addStretch();
    setLayout(layout);
}

TextWidget::~TextWidget()
{}