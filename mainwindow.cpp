#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
     ui->setupUi(this);

     Init();

     //test qtcreator
}


MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::Init_UartPort
 * 利用QSerialPortInfo获取端口信息，并更新
 */
void MainWindow::Init_UartPort(){

    ui->comBox_uartPort->clear();

    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->comBox_uartPort->addItem(info.portName());
    }

}

///**
// * @brief MainWindow::Init_UartDps
// * 利用QSerialPortInfo获取系统支持的波特率，并更新
// */
//void MainWindow::Init_UartDps(){

//    ui->comBox_uartDps->clear();

//    foreach (const qint32 &baudrate,  QSerialPortInfo::standardBaudRates()) {
//        ui->comBox_uartDps->addItem(QString::number(baudrate));
//    }

//    ui->comBox_uartDps->setCurrentIndex(7);

//}

void MainWindow::Init_UI(){

    //仅能输入大于0的数字
    ui->lineEdit_AutoResend->setValidator(new QIntValidator(0, INT_MAX, this));

    ui->comBox_uartDataLen->setCurrentIndex(3);
    ui->comBox_uartDps->setCurrentIndex(7);
    ui->textBrowser_intput->clear();

    ui->radioButton_displayASCII->setChecked(true);
    ui->radioButton_sendoutASCII->setChecked(true);

    ui->pushButton_uartConnect->setEnabled(true);
    ui->pushButton_uartDisconnect->setEnabled(false);
    ui->buttom_sendout->setEnabled(false);

    ui->checkBox_displayNewline->setChecked(true);

    Init_UartPort();
//    Init_UartDps();

}

/**
 * @brief MainWindow::Init
 * 变量+UI初始化
 */
void MainWindow::Init(){

    uart_state = s_disconnect;
    mAutosendoutTimer = new QTimer(this);
    connect(mAutosendoutTimer,SIGNAL(timeout()),this,SLOT(slot_Timeout_Uartsendout()));

    mBGdisplaymode = new QButtonGroup(this);
    mBGdisplaymode->addButton(ui->radioButton_displayASCII);
    mBGdisplaymode->addButton(ui->radioButton_displayHEX);
    mBGdisplaymode->setId(ui->radioButton_displayASCII,DISPLAYMODE_ASCII);
    mBGdisplaymode->setId(ui->radioButton_displayHEX,DISPLAYMODE_HEX);

    mBGsendoutmode = new QButtonGroup(this);
    mBGsendoutmode->addButton(ui->radioButton_sendoutASCII);
    mBGsendoutmode->addButton(ui->radioButton_sendoutHEX);
    mBGsendoutmode->setId(ui->radioButton_sendoutASCII,SENDOUTMODE_ASCII);
    mBGsendoutmode->setId(ui->radioButton_sendoutHEX,SENDOUTMODE_HEX);

    Init_UI();

    connect(ui->comBox_uartPort,SIGNAL(clicked()),this,SLOT(on_comBox_uartPort_clicked()));

}

/**
 * @brief MainWindow::on_pushButton_uartDisconnect_clicked
 * 断开串口连接 - 槽函数
 */
void MainWindow::on_pushButton_uartDisconnect_clicked()
{
    if(s_connect == uart_state){
        mSerial->close();
        Init();
        setNewsColor(Qt::black);
        ui->label_news->setText("SerialPort Close Success!");
    }
}

/**
 * @brief MainWindow::on_pushButton_uartConnect_clicked
 * 串口连接 - 槽函数
 */
void MainWindow::on_pushButton_uartConnect_clicked()
{
    //获取串口参数并设置
    mSerial = new QSerialPort();

    QString mPortName = ui->comBox_uartPort->currentText();
    QSerialPort::BaudRate mBaudRate = (QSerialPort::BaudRate)ui->comBox_uartDps->currentText().toInt();
    QSerialPort::DataBits mDataBits = (QSerialPort::DataBits)ui->comBox_uartDataLen->currentText().toInt();
    QSerialPort::FlowControl mFlowControl = (QSerialPort::FlowControl)ui->comBox_uartFlowControl->currentIndex();

    QSerialPort::Parity mParity;
    if(0 == ui->comBox_uartCheckBit->currentIndex()){
       mParity = QSerialPort::NoParity;
    }
    else{
        mParity = (QSerialPort::Parity)(ui->comBox_uartCheckBit->currentIndex() + 1);
    }
    QSerialPort::StopBits mStopBits = (QSerialPort::StopBits)(ui->comBox_uartStopBit->currentIndex() + 1);

    qDebug() << mPortName;
    qDebug() << mBaudRate;
    qDebug() << mDataBits;
    qDebug() << mFlowControl;
    qDebug() << mParity;
    qDebug() << mStopBits;

    mSerial->setPortName(mPortName);
    mSerial->setBaudRate(mBaudRate);
    mSerial->setDataBits(mDataBits);
    mSerial->setFlowControl(mFlowControl);
    mSerial->setParity(mParity);
    mSerial->setStopBits(mStopBits);

    if(mSerial->open((QIODevice::ReadWrite))){
        uart_state = s_connect;
        ui->pushButton_uartConnect->setEnabled(false);
        ui->pushButton_uartDisconnect->setEnabled(true);
        ui->buttom_sendout->setEnabled(true);

        //清缓存
        mSerial->clear();

        connect(mSerial,SIGNAL(readyRead()),this,SLOT(slot_uartReadData()));
        connect(mSerial,SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
                this,SLOT(slot_uartError(QSerialPort::SerialPortError)));
        setNewsColor(Qt::black);
        ui->label_news->setText("SerialPort Open Success!");
    }
    else{
        uart_state = s_disconnect;
        setNewsColor(Qt::red);
        ui->label_news->setText("SerialPort Open Fail!");
    }
}

/**
 * @brief MainWindow::on_buttom_sendout_clicked
 * 串口数据发送 - 槽函数
 */
void MainWindow::on_buttom_sendout_clicked()
{
    if(uart_state != s_connect){
        setNewsColor(Qt::red);
        ui->label_news->setText("SerialPort is't Connect.");
        return;
    }

    if(mBGsendoutmode->checkedId() == SENDOUTMODE_ASCII){
        //支持中文需使用toLocal8Bit()
        mSerial->write(ui->textEdit_output->toPlainText().toLocal8Bit().data());
    }
    else if(mBGsendoutmode->checkedId() == SENDOUTMODE_HEX){
        QByteArray bytehex = QByteArray::fromHex(
                    ui->textEdit_output->toPlainText().toLatin1()).data();
         mSerial->write(bytehex);
    }
}

/**
 * @brief MainWindow::on_comBox_uartPort_clicked
 * 点击comBox_uartPort - 槽函数
 * 用以更新端口信息
 */
void MainWindow::on_comBox_uartPort_clicked(){
    Init_UartPort();
}

/**
 * @brief MainWindow::slot_Timeout_Uartsendout
 * 定时重发 - 定时间超时槽函数
 */
void MainWindow::slot_Timeout_Uartsendout(){
    on_buttom_sendout_clicked();
}

/**
 * @brief MainWindow::slot_uartReadData
 * 串口接收数据 - 槽函数
 */
void MainWindow::slot_uartReadData(){

    if(ui->checkBox_displayNewline->isChecked()){
        if(ui->textBrowser_intput->document()->toPlainText() != "")
            ui->textBrowser_intput->insertPlainText("\r\n");
    }

    if(ui->checkBox_displayTime->isChecked()){
        ui->textBrowser_intput->insertPlainText(QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] "));
    }

    if(DISPLAYMODE_ASCII == mBGdisplaymode->checkedId()){

        ui->textBrowser_intput->insertPlainText(QString::fromLocal8Bit(mSerial->readAll()));

    }
    // This Error！！！
    else if(DISPLAYMODE_HEX == mBGdisplaymode->checkedId()){

        QString re = "";

        QByteArray bytearray = mSerial->readAll();

        //hex char[] 转QString

        for(int i = 0; i < bytearray.length(); i++){
            if((unsigned char)bytearray[i] > 255)
                re.append("Error ");
            re.append("0x" + QString::number((unsigned char)bytearray[i],16) + " ");
        }

        ui->textBrowser_intput->insertPlainText(re);
    }

}

/**
 * @brief MainWindow::slot_uartError
 * @param error
 * 串口出错 - 槽函数
 * 用以显示错误信息
 */
void MainWindow::slot_uartError(QSerialPort::SerialPortError error){

    QMetaEnum metaError = QMetaEnum::fromType<QSerialPort::SerialPortError>();
    ui->label_news->setText(metaError.valueToKey(error));
    setNewsColor(Qt::red);
}

/**
 * @brief MainWindow::on_checkBox_sendoutAutoResend_clicked
 * @param checked
 * 定时重发功能 - 槽函数
 */
void MainWindow::on_checkBox_sendoutAutoResend_clicked(bool checked)
{
    if(checked){

        if(s_connect == uart_state){
            mAutosendoutTimer->start(ui->lineEdit_AutoResend->text().toInt());
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort is't Connect.");
            return;
        }
    }
    else{
        if(mAutosendoutTimer->isActive())
            mAutosendoutTimer->stop();
    }
}

/**
 * @brief MainWindow::on_lineEdit_AutoResend_textChanged
 * @param arg1
 * 更改定时重发时间 - 槽函数
 * 为支持开启定时重发状态更改定时时间
 */
void MainWindow::on_lineEdit_AutoResend_textChanged(const QString &arg1)
{
    if(ui->checkBox_sendoutAutoResend->isChecked()){
        if(mAutosendoutTimer->isActive()){
            mAutosendoutTimer->stop();
            mAutosendoutTimer->start(arg1.toInt());
        }
    }
}

/**
 * @brief MainWindow::setNewsColor
 * @param color
 * 设置News提醒颜色
 */
void MainWindow::setNewsColor(Qt::GlobalColor color){
    QPalette pa;
    pa.setColor(QPalette::WindowText,color);
    ui->label_news->setPalette(pa);
}

void MainWindow::on_pushButton_clear_TBinput_clicked()
{
    ui->textBrowser_intput->clear();
}

void MainWindow::on_pushButton_clear_TBoutput_clicked()
{
    ui->textEdit_output->clear();
}

/**
 * 以下用于开启串口后设置串口参数
 */
void MainWindow::on_comBox_uartDps_currentTextChanged(const QString &arg1)
{
    if(s_connect == uart_state){

        //自定义波特率
        if(ui->comBox_uartDps->currentIndex() == 8){
            return;
        }

        QSerialPort::BaudRate mBaudRate = (QSerialPort::BaudRate)arg1.toInt();

        if(mSerial->setBaudRate(mBaudRate)){
            setNewsColor(Qt::black);
            ui->label_news->setText("SerialPort setBaudRate is OK.");
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort setBaudRate is Error.");
        }
    }

}

void MainWindow::on_comBox_uartDataLen_currentIndexChanged(const QString &arg1)
{
    if(s_connect == uart_state){

        QSerialPort::DataBits mDataBits = (QSerialPort::DataBits)arg1.toInt();

        if(mSerial->setDataBits(mDataBits)){
            setNewsColor(Qt::black);
            ui->label_news->setText("SerialPort setDataBits is OK.");
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort setDataBits is Error.");
        }
    }
}

void MainWindow::on_comBox_uartCheckBit_currentIndexChanged(int index)
{
    if(s_connect == uart_state){

        QSerialPort::Parity mParity;
        if(0 == index){
           mParity = QSerialPort::NoParity;
        }
        else{
            mParity = (QSerialPort::Parity)(index + 1);
        }


        if(mSerial->setParity(mParity)){
            setNewsColor(Qt::black);
            ui->label_news->setText("SerialPort setParity is OK.");
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort setParity is Error.");
        }
    }
}



void MainWindow::on_comBox_uartFlowControl_currentIndexChanged(int index)
{
    if(s_connect == uart_state){

        QSerialPort::FlowControl mFlowControl = (QSerialPort::FlowControl)index;


        if(mSerial->setFlowControl(mFlowControl)){
            setNewsColor(Qt::black);
            ui->label_news->setText("SerialPort setFlowControl is OK.");
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort setFlowControl is Error.");
        }
    }
}

void MainWindow::on_comBox_uartStopBit_currentIndexChanged(int index)
{
    if(s_connect == uart_state){

         QSerialPort::StopBits mStopBits = (QSerialPort::StopBits)(index + 1);


        if(mSerial->setStopBits(mStopBits)){
            setNewsColor(Qt::black);
            ui->label_news->setText("SerialPort setStopBits is OK.");
        }
        else{
            setNewsColor(Qt::red);
            ui->label_news->setText("SerialPort setStopBits is Error.");
        }
    }
}

void MainWindow::on_comBox_uartDps_currentIndexChanged(int index)
{
    //自定义波特率
    if(8 == index){
        QLineEdit *lineEdit = new QLineEdit(this);
        ui->comBox_uartDps->setLineEdit(lineEdit);

        lineEdit->clear();
        lineEdit->setFocus();

        //正则限制输入
        QRegExp rx("[0-9]+$");
        QRegExpValidator *validator = new QRegExpValidator(rx, this);
        lineEdit->setValidator(validator);
    }
    else{
        ui->comBox_uartDps->setEditable(false);
    }
}
