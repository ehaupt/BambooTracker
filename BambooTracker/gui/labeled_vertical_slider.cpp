#include "labeled_vertical_slider.hpp"
#include "ui_labeled_vertical_slider.h"
#include <QWheelEvent>
#include "slider_style.hpp"

LabeledVerticalSlider::LabeledVerticalSlider(QWidget *parent) :
	QFrame(parent),
	ui(new Ui::LabeledVerticalSlider)
{
	init("", "", "");
}

LabeledVerticalSlider::LabeledVerticalSlider(QString text, QString prefix, QString suffix, QWidget *parent) :
	QFrame(parent),
	ui(new Ui::LabeledVerticalSlider)
{
	init(text, prefix, suffix);
}

void LabeledVerticalSlider::init(QString text, QString prefix, QString suffix)
{
	ui->setupUi(this);

	rate_ = 1.0;
	precision_ = 0;
	isSigned_ = false;
	ui->textLabel->setText(text);
	prefix_ = prefix;
	suffix_ = suffix;
	updateValueLabel();
	ui->slider->setStyle(new SliderStyle());
	ui->slider->installEventFilter(this);
}

LabeledVerticalSlider::~LabeledVerticalSlider()
{
	delete ui;
}

int LabeledVerticalSlider::value() const
{
	return ui->slider->value();
}

void LabeledVerticalSlider::setValue(int value)
{
	ui->slider->setValue(value);
}

int LabeledVerticalSlider::maximum() const
{
	return ui->slider->maximum();
}

void LabeledVerticalSlider::setMaximum(int value)
{
	ui->slider->setMaximum(value);
}

int LabeledVerticalSlider::minimum() const
{
	return ui->slider->minimum();
}

void LabeledVerticalSlider::setMinimum(int value)
{
	ui->slider->setMinimum(value);
}

void LabeledVerticalSlider::setValueRate(double rate, int precision)
{
	rate_ = rate;
	precision_ = precision;
	updateValueLabel();
}

void LabeledVerticalSlider::setSign(bool enabled)
{
	isSigned_ = enabled;
	updateValueLabel();
}

void LabeledVerticalSlider::setTickPosition(QSlider::TickPosition position)
{
	ui->slider->setTickPosition(position);
}

void LabeledVerticalSlider::setTickInterval(int ti)
{
	ui->slider->setTickInterval(ti);
}

QString LabeledVerticalSlider::text() const
{
	return ui->textLabel->text();
}

void LabeledVerticalSlider::setText(QString text)
{
	ui->textLabel->setText(text);
}

QString LabeledVerticalSlider::suffix() const
{
	return suffix_;
}

void LabeledVerticalSlider::setSuffix(QString suffix)
{
	suffix_ = suffix;
	updateValueLabel();
}

QString LabeledVerticalSlider::prefix() const
{
	return prefix_;
}

void LabeledVerticalSlider::setprefix(QString prefix)
{
	prefix_ = prefix;
	updateValueLabel();
}

bool LabeledVerticalSlider::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == ui->slider) {
		if (event->type() == QEvent::Wheel) {
			auto e = dynamic_cast<QWheelEvent*>(event);
			if (e->angleDelta().y() > 0) ui->slider->setValue(ui->slider->value() + 1);
			else if (e->angleDelta().y() < 0) ui->slider->setValue(ui->slider->value() - 1);
			return true;
		}
	}

	return QFrame::eventFilter(watched, event);
}

void LabeledVerticalSlider::on_slider_valueChanged(int value)
{
	updateValueLabel();
	emit valueChanged(value);
}

void LabeledVerticalSlider::updateValueLabel()
{
	QString sign = (isSigned_ && ui->slider->value() > -1) ? "+" : "";
	ui->valueLabel->setText(
				prefix_ + sign + QString::number(ui->slider->value() * rate_, 'f', precision_) + suffix_);
}
