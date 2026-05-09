#include <QApplication>
#include <QWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDate>
#include <QFileDialog>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QSplitter>

class FeynmanNoteGenerator : public QWidget {
    Q_OBJECT

public:
    FeynmanNoteGenerator(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
        loadSettings();
    }

private:
    QLineEdit *projectEdit;
    QTextEdit *questionEdit;
    QTextEdit *explanationEdit;
    QDateEdit *dateEdit;
    QLineEdit *outputDirEdit;
    QCheckBox *defaultDirCheck;
    QTextEdit *previewEdit;
    QCheckBox *renderMarkdownCheck;

    void setupUI() {
        setWindowTitle("费曼笔记生成器");

        // 日期选择器
        dateEdit = new QDateEdit(QDate::currentDate());
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat("yyyy-MM-dd");

        // 项目和问题输入
        projectEdit = new QLineEdit();
        projectEdit->setPlaceholderText("例如：我的桌面整理器");

        questionEdit = new QTextEdit();
        questionEdit->setPlaceholderText("你遇到了什么问题？");
        questionEdit->setMaximumHeight(80);

        explanationEdit = new QTextEdit();
        explanationEdit->setPlaceholderText("用你自己的话解释这个知识点...");

        // 输出目录选择
        outputDirEdit = new QLineEdit();
        outputDirEdit->setPlaceholderText("选择导出目录...");
        outputDirEdit->setReadOnly(true);

        QPushButton *browseBtn = new QPushButton("浏览...");
        connect(browseBtn, &QPushButton::clicked, this, &FeynmanNoteGenerator::browseOutputDir);

        QHBoxLayout *dirLayout = new QHBoxLayout();
        dirLayout->addWidget(outputDirEdit);
        dirLayout->addWidget(browseBtn);

        // 默认目录勾选框
        defaultDirCheck = new QCheckBox("设为默认导出目录");
        connect(defaultDirCheck, &QCheckBox::toggled, this, &FeynmanNoteGenerator::onDefaultDirToggled);

        // 按钮
        QPushButton *generateBtn = new QPushButton("生成 Markdown 笔记");
        connect(generateBtn, &QPushButton::clicked, this, &FeynmanNoteGenerator::generateNote);

        // 让 form 字段水平铺满
        dateEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        projectEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        questionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        explanationEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 用全角空格在字符间填充，仅填充冒号前的文字部分
        auto pad = [](const QString &body, int targetLen) -> QString {
            if (body.length() >= targetLen) return body + "：";
            int gaps = body.length() - 1;
            int toAdd = targetLen - body.length();
            QString result;
            for (int i = 0; i < body.length(); ++i) {
                result += body[i];
                if (i < body.length() - 1) {
                    int n = toAdd / gaps + (i < toAdd % gaps ? 1 : 0);
                    result += QString(n, QChar(0x3000));
                }
            }
            return result + "：";
        };
        const int maxBodyLen = 4; // "所属项目" 字符数

        // 布局
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->addRow(pad("日期", maxBodyLen), dateEdit);
        formLayout->addRow(pad("所属项目", maxBodyLen), projectEdit);
        formLayout->addRow(pad("问题", maxBodyLen), questionEdit);
        formLayout->addRow(pad("我的理解", maxBodyLen), explanationEdit);
        formLayout->addRow(pad("输出目录", maxBodyLen), dirLayout);
        formLayout->addRow("", defaultDirCheck);

        // 右侧实时预览
        previewEdit = new QTextEdit();
        previewEdit->setReadOnly(true);
        previewEdit->setPlaceholderText("实时预览...");

        renderMarkdownCheck = new QCheckBox("渲染 Markdown");
        renderMarkdownCheck->setChecked(false);
        connect(renderMarkdownCheck, &QCheckBox::toggled, this, &FeynmanNoteGenerator::refreshPreview);

        QWidget *rightPanel = new QWidget();
        rightPanel->setMinimumWidth(200);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->addWidget(renderMarkdownCheck);
        rightLayout->addWidget(previewEdit);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        // 左栏：输入表单
        QWidget *leftPanel = new QWidget();
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->addLayout(formLayout);
        leftLayout->addWidget(generateBtn);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        // 左右分栏
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
        splitter->addWidget(leftPanel);
        splitter->addWidget(rightPanel);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(splitter);
        setLayout(mainLayout);

        // 信号：输入变化 → 实时刷新预览
        connect(dateEdit, &QDateEdit::dateChanged, this, &FeynmanNoteGenerator::refreshPreview);
        connect(projectEdit, &QLineEdit::textChanged, this, &FeynmanNoteGenerator::refreshPreview);
        connect(questionEdit, &QTextEdit::textChanged, this, &FeynmanNoteGenerator::refreshPreview);
        connect(explanationEdit, &QTextEdit::textChanged, this, &FeynmanNoteGenerator::refreshPreview);
        connect(outputDirEdit, &QLineEdit::textChanged, this, &FeynmanNoteGenerator::refreshPreview);

        refreshPreview();
    }

    void browseOutputDir() {
        QString dir = QFileDialog::getExistingDirectory(this, "选择输出目录",
                                                         outputDirEdit->text().isEmpty()
                                                             ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                                                             : outputDirEdit->text());
        if (!dir.isEmpty()) {
            outputDirEdit->setText(dir);
        }
    }

    void onDefaultDirToggled(bool checked) {
        QSettings settings;
        if (checked) {
            settings.setValue("defaultOutputDir", outputDirEdit->text());
        } else {
            settings.remove("defaultOutputDir");
        }
    }

    void loadSettings() {
        QSettings settings;
        QString defaultDir = settings.value("defaultOutputDir").toString();
        if (!defaultDir.isEmpty()) {
            outputDirEdit->setText(defaultDir);
            defaultDirCheck->setChecked(true);
        } else {
            outputDirEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        }
    }

    QString outputDir() const {
        QString dir = outputDirEdit->text().trimmed();
        if (dir.isEmpty()) {
            dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }
        return dir;
    }

    QString buildMarkdown() const {
        QString date = dateEdit->date().toString("yyyy-MM-dd");
        QString project = projectEdit->text().trimmed();
        QString question = questionEdit->toPlainText().trimmed();
        QString explanation = explanationEdit->toPlainText().trimmed();

        return QStringLiteral(
            "---\n"
            "date: %1\n"
            "project: %2\n"
            "tags: []\n"
            "---\n\n"
            "# 问题\n%3\n\n"
            "# 用自己的话说\n%4\n\n"
            "# 代码片段 (可选)\n\n\n"
            "# AI 的苏格拉底式拷问 (复习时填写)\n- Q: \n- A: \n"
        ).arg(date, project, question, explanation);
    }

    void refreshPreview() {
        if (renderMarkdownCheck->isChecked()) {
            previewEdit->setMarkdown(buildMarkdown());
        } else {
            previewEdit->setPlainText(buildMarkdown());
        }
    }

    void generateNote() {
        QString date = dateEdit->date().toString("yyyy-MM-dd");
        QString project = projectEdit->text().trimmed();
        QString question = questionEdit->toPlainText().trimmed();
        QString explanation = explanationEdit->toPlainText().trimmed();

        if (project.isEmpty() || question.isEmpty() || explanation.isEmpty()) {
            QMessageBox::warning(this, "信息不完整", "请填写项目、问题和你的理解。");
            return;
        }

        QString mdContent = buildMarkdown();

        // 生成文件名
        QString filename = date + "-" + project + ".md";
        filename.replace(' ', '_');

        // 保存到选择的目录
        QString filePath = outputDir() + "/" + filename;

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << mdContent;
            file.close();
            QMessageBox::information(this, "成功",
                                     QString("笔记已保存到：\n%1").arg(filePath));
        } else {
            QMessageBox::critical(this, "错误", "无法创建文件，请检查目录权限。");
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("quicknote");
    app.setApplicationName("quicknote");

    FeynmanNoteGenerator window;
    window.resize(800, 500);
    window.show();

    return app.exec();
}

#include "main.moc"
